// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <Security/SecureTransport.h>

struct tls_write {
	void *out;
	size_t size;
	size_t *written;
};

struct MTY_Cert {
	bool dummy;
};

struct MTY_TLS {
	char *fp;

	uint8_t *buf;
	size_t size;
	size_t offset;

	void *opaque;
	struct tls_write *w;
	MTY_TLSWriteFunc write_func;

	SSLContextRef ctx;
};

#define TLS_MTU_PADDING 64


// CACert

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	// Unnecessary, always use the Apple's CACert store

	return false;
}


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	// TODO Needed only for DTLS

	return MTY_Alloc(1, sizeof(MTY_Cert));
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	// TODO Needed only for DTLS

	memset(fingerprint, 0, size);
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	MTY_Free(ctx);
	*cert = NULL;
}


// TLS, DTLS

static OSStatus tls_read_func(SSLConnectionRef connection, void *data, size_t *dataLength)
{
	MTY_TLS *ctx = (MTY_TLS *) connection;

	if (ctx->offset < *dataLength)
		*dataLength = ctx->offset;

	memcpy(data, ctx->buf, *dataLength);
	ctx->offset -= *dataLength;

	memmove(ctx->buf, ctx->buf + *dataLength, ctx->offset);

	return *dataLength == 0 ? errSSLWouldBlock : noErr;
}

static OSStatus tls_write_func(SSLConnectionRef connection, const void *data, size_t *dataLength)
{
	MTY_TLS *ctx = (MTY_TLS *) connection;

	// Encrypt
	if (ctx->w) {
		if (ctx->w->size < *dataLength)
			*dataLength = ctx->w->size;

		memcpy(ctx->w->out, data, *dataLength);
		*ctx->w->written = *dataLength;

		return noErr;
	}

	// Handshake
	return ctx->write_func(data, *dataLength, ctx->opaque) ? noErr : errSSLNetworkTimeout;
}

MTY_TLS *MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu)
{
	bool r = true;

	MTY_TLS *ctx = MTY_Alloc(1, sizeof(MTY_TLS));

	ctx->ctx = SSLCreateContext(NULL, kSSLClientSide, type == MTY_TLS_TYPE_DTLS ? kSSLDatagramType : kSSLStreamType);
	if (!ctx->ctx) {
		MTY_Log("'SSLCreateContext' failed");
		r = false;
		goto except;
	}

	OSStatus e = SSLSetProtocolVersionMin(ctx->ctx, type == MTY_TLS_TYPE_DTLS ? kTLSProtocol12 : kDTLSProtocol12);
	if (e != noErr) {
		MTY_Log("'SSLSetProtocolVersionMin' failed with error %d", e);
		r = false;
		goto except;
	}

	e = SSLSetIOFuncs(ctx->ctx, tls_read_func, tls_write_func);
	if (e != noErr) {
		MTY_Log("'SSLSetIOFuncs' failed with error %d", e);
		r = false;
		goto except;
	}

	e = SSLSetConnection(ctx->ctx, ctx);
	if (e != noErr) {
		MTY_Log("'SSLSetConnection' failed with error %d", e);
		r = false;
		goto except;
	}

	if (host) {
		e = SSLSetPeerDomainName(ctx->ctx, host, strlen(host));
		if (e != noErr) {
			MTY_Log("'SSLSetPeerDomainName' failed with error %d", e);
			r = false;
			goto except;
		}
	}

	if (type == MTY_TLS_TYPE_DTLS) {
		e = SSLSetMaxDatagramRecordSize(ctx->ctx, mtu + TLS_MTU_PADDING);
		if (e != noErr) {
			MTY_Log("'SSLSetMaxDatagramRecordSize' failed with error %d", e);
			r = false;
			goto except;
		}
	}

	if (cert) {
		// TODO Only needed for DTLS
	}

	if (peerFingerprint)
		ctx->fp = MTY_Strdup(peerFingerprint);

	except:

	if (!r)
		MTY_TLSDestroy(&ctx, NULL, 0, NULL);

	return ctx;
}

void MTY_TLSDestroy(MTY_TLS **tls, void *buf, size_t size, size_t *written)
{
	if (!tls || !*tls)
		return;

	MTY_TLS *ctx = *tls;

	if (ctx->ctx)
		CFRelease(ctx->ctx);

	MTY_Free(ctx->fp);

	MTY_Free(ctx);
	*tls = NULL;
}

static void tls_add_data(MTY_TLS *ctx, const void *buf, size_t size)
{
	if (size + ctx->offset > ctx->size) {
		ctx->size = size + ctx->offset;
		ctx->buf = MTY_Realloc(ctx->buf, ctx->size, 1);
	}

	memcpy(ctx->buf + ctx->offset, buf, size);
	ctx->offset += size;
}

MTY_Async MTY_TLSHandshake(MTY_TLS *ctx, const void *buf, size_t size, MTY_TLSWriteFunc writeFunc, void *opaque)
{
	if (buf && size > 0)
		tls_add_data(ctx, buf, size);

	ctx->w = NULL;
	ctx->opaque = opaque;
	ctx->write_func = writeFunc;

	OSStatus e = SSLHandshake(ctx->ctx);

	if (e != errSSLWouldBlock && e != noErr)
		MTY_Log("'SSLHandshake' failed with error %d\n", e);

	return e == errSSLWouldBlock ? MTY_ASYNC_CONTINUE : e == noErr ? MTY_ASYNC_OK : MTY_ASYNC_ERROR;
}

bool MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	struct tls_write w = {0};
	w.out = out;
	w.size = outSize;
	w.written = written;

	ctx->w = &w;

	size_t processed = 0;
	OSStatus e = SSLWrite(ctx->ctx, in, inSize, &processed);

	return e == noErr;
}

bool MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	tls_add_data(ctx, in, inSize);

	OSStatus e = SSLRead(ctx->ctx, out, outSize, read);

	return e == noErr;
}


// 0x14   - Change Cipher Spec
// 0x16   - Handshake
// 0x17   - Application Data

// 0xFEFF - DTLS 1.0
// 0xFEFD - DTLS 1.2
// 0x0303 - TLS 1.2

bool MTY_TLSIsHandshake(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && (d[0] == 0x14 || d[0] == 0x16) && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}

bool MTY_TLSIsApplicationData(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && d[0] == 0x17 && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}
