// Copyright (c) 2021 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "net/sec.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SECURITY_WIN32
#include <windows.h>
#include <schannel.h>
#include <sspi.h>

#define TLS_BUF_SIZE (1 * 1024)

struct tls {
	TCP_SOCKET socket;

	CredHandle ch;
	CtxtHandle ctx;

	uint8_t *dbuf;
	uint8_t *rbuf;
	size_t pending;
	size_t rbuf_size;
	size_t dbuf_size;
};

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	return false;
}

void tls_destroy(struct tls **tls)
{
	if (!tls || !*tls)
		return;

	struct tls *ctx = *tls;

	DeleteSecurityContext(&ctx->ctx);
	FreeCredentialsHandle(&ctx->ch);

	MTY_Free(ctx);
	*tls = NULL;
}

static bool tls_read_message(struct tls *ctx, uint32_t timeout, size_t *size)
{
	if (!tcp_read(ctx->socket, ctx->dbuf, 5, timeout))
		return false;

	uint16_t len = 0;
	memcpy(&len, ctx->dbuf + 3, 2);
	len = MTY_SwapFromBE16(len);

	*size = len + 5;
	if (*size > ctx->dbuf_size) {
		ctx->dbuf = MTY_Realloc(ctx->dbuf, *size, 1);
		ctx->dbuf_size = *size;
	}

	return tcp_read(ctx->socket, ctx->dbuf + 5, len, timeout);
}

struct tls *tls_connect(TCP_SOCKET socket, const char *host, uint32_t timeout)
{
	bool r = true;
	struct tls *ctx = MTY_Alloc(1, sizeof(struct tls));
	ctx->socket = socket;
	ctx->dbuf = MTY_Alloc(5, 1); // Minimum TLS header size
	ctx->dbuf_size = 5;

	SecBuffer out = {0};
	out.BufferType = SECBUFFER_EMPTY;

	// Creds initialization
	SCHANNEL_CRED sc = {0};
	sc.dwVersion = SCHANNEL_CRED_VERSION;
	sc.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
	sc.dwFlags = SCH_USE_STRONG_CRYPTO;

	SECURITY_STATUS e = AcquireCredentialsHandle(NULL, L"Microsoft Unified Security Protocol Provider",
		SECPKG_CRED_OUTBOUND, NULL, &sc, NULL, NULL, &ctx->ch, NULL);
	if (e != SEC_E_OK) {
		MTY_Log("'AcquireCredentialsHandle' failed with error 0x%X", e);
		r = false;
		goto except;
	}

	// Handhake part 1 (Client Hello)
	WCHAR whost[512];
	_snwprintf_s(whost, 512, _TRUNCATE, L"%hs", host);

	unsigned long flags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
		ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	unsigned long out_flags = 0;

	SecBufferDesc out_desc = {0};
	out_desc.ulVersion = SECBUFFER_VERSION;
	out_desc.pBuffers = &out;
	out_desc.cBuffers = 1;

	e = InitializeSecurityContext(&ctx->ch, NULL, whost, flags, 0, 0, NULL, 0,
		&ctx->ctx, &out_desc, &out_flags, NULL);
	if (e != SEC_I_CONTINUE_NEEDED) {
		MTY_Log("'InitializeSecurityContext' failed with error 0x%X", e);
		r = false;
		goto except;
	}

	// TCP write the resulting Client Hello
	r = tcp_write(ctx->socket, out.pvBuffer, out.cbBuffer);
	if (!r)
		goto except;

	// Enter into the handshake loop
	while (true) {
		size_t size = 0;
		r = tls_read_message(ctx, timeout, &size);
		if (!r)
			goto except;

		// Set up input and output buffers
		SecBuffer in[2] = {0};
		in[0].BufferType = SECBUFFER_TOKEN;
		in[0].pvBuffer = ctx->dbuf;
		in[0].cbBuffer = (unsigned long) size;

		in[1].BufferType = SECBUFFER_EMPTY;

		SecBufferDesc in_desc = {0};
		in_desc.ulVersion = SECBUFFER_VERSION;
		in_desc.pBuffers = in;
		in_desc.cBuffers = 2;

		SecBuffer out2[3] = {0};
		out2[0].BufferType = SECBUFFER_TOKEN;
		out2[1].BufferType = SECBUFFER_ALERT;
		out2[2].BufferType = SECBUFFER_EMPTY;

		SecBufferDesc out_desc2 = {0};
		out_desc2.ulVersion = SECBUFFER_VERSION;
		out_desc2.pBuffers = out2;
		out_desc2.cBuffers = 3;

		// Feed the TLS handshake message to schannel
		e = InitializeSecurityContext(&ctx->ch, &ctx->ctx, whost, flags, 0, 0, &in_desc, 0,
			NULL, &out_desc2, &out_flags, NULL);

		// Check for errors
		if (e != SEC_E_OK && e != SEC_I_CONTINUE_NEEDED) {
			MTY_Log("'InitializeSecurityContext' failed with error 0x%X", e);
			r = false;
			goto except;
		}

		// Schannel has produced response handshake
		if (out2[0].cbBuffer > 0) {
			r = tcp_write(ctx->socket, out2[0].pvBuffer, out2[0].cbBuffer);

			FreeContextBuffer(out2[0].pvBuffer);

			if (!r)
				goto except;
		}

		// Handshake has completed successfully
		if (e == SEC_E_OK)
			break;
	}

	except:

	if (out.pvBuffer)
		FreeContextBuffer(out.pvBuffer);

	if (!r)
		tls_destroy(&ctx);

	return ctx;
}

struct tls *tls_accept(TCP_SOCKET socket, uint32_t timeout)
{
	return NULL;
}

bool tls_write(struct tls *ctx, const void *buf, size_t size)
{
	SecPkgContext_StreamSizes sizes = {0};
	SECURITY_STATUS e = QueryContextAttributes(&ctx->ctx, SECPKG_ATTR_STREAM_SIZES, &sizes);
	if (e != SEC_E_OK) {
		MTY_Log("'QueryContextAttributes' failed with error 0x%X", e);
		return false;
	}

	if (size > sizes.cbMaximumMessage) {
		MTY_Log("TLS message size is too large (%zu > %u)", size, sizes.cbMaximumMessage);
		return false;
	}

	bool r = true;
	size_t full_size = sizes.cbHeader + size + sizes.cbTrailer;
	uint8_t *tmp = MTY_Alloc(full_size, 1);
	memcpy(tmp + sizes.cbHeader, buf, size);

	SecBuffer bufs[4] = {0};
	bufs[0].BufferType = SECBUFFER_STREAM_HEADER;
	bufs[0].pvBuffer = tmp;
	bufs[0].cbBuffer = sizes.cbHeader;

	bufs[1].BufferType = SECBUFFER_DATA;
	bufs[1].pvBuffer = tmp + sizes.cbHeader;
	bufs[1].cbBuffer = (unsigned long) size;

	bufs[2].BufferType = SECBUFFER_STREAM_TRAILER;
	bufs[2].pvBuffer = tmp + sizes.cbHeader + size;;
	bufs[2].cbBuffer = sizes.cbTrailer;

	bufs[3].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc desc = {0};
	desc.ulVersion = SECBUFFER_VERSION;
	desc.pBuffers = bufs;
	desc.cBuffers = 4;

	e = EncryptMessage(&ctx->ctx, 0, &desc, 0);
	if (e != SEC_E_OK) {
		MTY_Log("'EncryptMessage' failed with error 0x%X", e);
		r = false;
		goto except;
	}

	// The first two buffers remain contiguous, but the trailer size may change
	size_t final = bufs[0].cbBuffer + bufs[1].cbBuffer + bufs[2].cbBuffer;

	r = tcp_write(ctx->socket, tmp, final);
	if (!r)
		goto except;

	except:

	MTY_Free(tmp);

	return r;
}

bool tls_read(struct tls *ctx, void *buf, size_t size, uint32_t timeout)
{
	while (true) {
		// Check if we have decrypted data in our buffer from a previous read
		if (ctx->pending >= size) {
			memcpy(buf, ctx->rbuf, size);
			ctx->pending -= size;

			memmove(ctx->rbuf, ctx->rbuf + size, ctx->pending);

			return true;
		}

		// We need more data, read a TLS message from the socket
		size_t msg_size = 0;
		if (!tls_read_message(ctx, timeout, &msg_size))
			return false;

		SecBuffer bufs[4] = {0};
		bufs[0].BufferType = SECBUFFER_DATA;
		bufs[0].pvBuffer = ctx->dbuf;
		bufs[0].cbBuffer = (unsigned long) msg_size;

		bufs[1].BufferType = SECBUFFER_EMPTY;
		bufs[2].BufferType = SECBUFFER_EMPTY;
		bufs[3].BufferType = SECBUFFER_EMPTY;

		SecBufferDesc desc = {0};
		desc.ulVersion = SECBUFFER_VERSION;
		desc.pBuffers = bufs;
		desc.cBuffers = 4;

		SECURITY_STATUS e = DecryptMessage(&ctx->ctx, &desc, 0, NULL);
		if (e != SEC_E_OK) {
			MTY_Log("'DecryptMessage' failed with error 0x%X", e);
			return false;
		}

		if (ctx->pending + bufs[1].cbBuffer > ctx->rbuf_size) {
			ctx->rbuf_size = ctx->pending + bufs[1].cbBuffer;
			ctx->rbuf = MTY_Realloc(ctx->rbuf, ctx->rbuf_size, 1);
		}

		memcpy(ctx->rbuf + ctx->pending, bufs[1].pvBuffer, bufs[1].cbBuffer);
		ctx->pending += bufs[1].cbBuffer;
	}

	return false;
}
