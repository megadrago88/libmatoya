// Copyright (c) 2021 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sec-engine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SECURITY_WIN32
#include <windows.h>
#include <schannel.h>
#include <sspi.h>

struct tls_cert {
	const CERT_CONTEXT *cert;
};

struct tls_engine {
	WCHAR *host;
	char *fp;

	CredHandle ch;
	CtxtHandle ctx;

	ULONG mtu;
	unsigned long flags;
	bool ctx_init;
	bool dtls;
};

#define TLS_MTU_PADDING 64

#define TLS_PROVIDER L"Microsoft Unified Security Protocol Provider"


// CACert

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	// Unnecessary, always use the Windows' CACert store

	return false;
}


// Self signed cert

struct tls_cert *tls_engine_cert_create(void)
{
	struct tls_cert *ctx = MTY_Alloc(1, sizeof(struct tls_cert));

	uint8_t rand_name[16];
	MTY_RandomBytes(rand_name, 16);

	char rand_name_hex[64] = "CN=";
	MTY_BytesToHex(rand_name, 16, rand_name_hex + 3, 61);

	WCHAR rand_name_hex_w[64];
	MTY_MultiToWide(rand_name_hex, rand_name_hex_w, 64);

	BYTE x509[256];
	DWORD x509_size = 256;
	CertStrToName(X509_ASN_ENCODING, rand_name_hex_w, CERT_X500_NAME_STR, NULL, x509, &x509_size, NULL);

	CERT_NAME_BLOB name = {0};
	name.cbData = x509_size;
	name.pbData = x509;

	CRYPT_ALGORITHM_IDENTIFIER algo = {0};
	algo.pszObjId = szOID_RSA_SHA256RSA;

	ctx->cert = CertCreateSelfSignCertificate((HCRYPTPROV_OR_NCRYPT_KEY_HANDLE) NULL,
		&name, 0, NULL, &algo, NULL, NULL, NULL);

	return ctx;
}

void tls_engine_cert_destroy(struct tls_cert **cert)
{
	if (!cert || !*cert)
		return;

	struct tls_cert *ctx = *cert;

	if (ctx->cert)
		CertFreeCertificateContext(ctx->cert);

	MTY_Free(ctx);
	*cert = NULL;
}

static void tls_engine_cert_context_to_fingerprint(const CERT_CONTEXT *cert, char *fingerprint, size_t size)
{
	memset(fingerprint, 0, size);

	uint8_t buf[MTY_SHA256_SIZE];

	MTY_CryptoHash(MTY_ALGORITHM_SHA256, cert->pbCertEncoded, cert->cbCertEncoded,
		NULL, 0, buf, MTY_SHA256_SIZE);

	MTY_Strcat(fingerprint, size, "sha-256 ");

	for (size_t x = 0; x < MTY_SHA256_SIZE; x++) {
		char append[8];
		snprintf(append, 8, "%02X:", buf[x]);
		MTY_Strcat(fingerprint, size, append);
	}

	// Remove the trailing ':'
	fingerprint[strlen(fingerprint) - 1] = '\0';
}

void tls_engine_cert_get_fingerprint(struct tls_cert *cert, char *fingerprint, size_t size)
{
	tls_engine_cert_context_to_fingerprint(cert->cert, fingerprint, size);
}


// TLS, DTLS

void tls_engine_destroy(struct tls_engine **engine)
{
	if (!engine || !*engine)
		return;

	struct tls_engine *ctx = *engine;

	DeleteSecurityContext(&ctx->ctx);
	FreeCredentialsHandle(&ctx->ch);

	MTY_Free(ctx->host);
	MTY_Free(ctx->fp);

	MTY_Free(ctx);
	*engine = NULL;
}

struct tls_engine *tls_engine_create(bool dtls, const char *host, const char *verify_fingerprint,
	struct tls_cert *cert, uint32_t mtu)
{
	struct tls_engine *ctx = MTY_Alloc(1, sizeof(struct tls_engine));
	ctx->dtls = dtls;

	// Even if the message is exactly cbMaximumMessage, EncryptMessage can still fail
	// Give it a bit of extra padding for presumably header/trailer
	ctx->mtu = mtu + TLS_MTU_PADDING;

	if (host)
		ctx->host = MTY_MultiToWideD(host);

	if (verify_fingerprint)
		ctx->fp = MTY_Strdup(verify_fingerprint);

	ctx->flags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
		ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	SCHANNEL_CRED sc = {0};
	sc.dwVersion = SCHANNEL_CRED_VERSION;
	sc.dwFlags = SCH_USE_STRONG_CRYPTO;

	if (dtls) {
		sc.grbitEnabledProtocols = SP_PROT_DTLS1_2_CLIENT;
		ctx->flags |= ISC_REQ_DATAGRAM;

		// DTLS uses self signed certificates, so disable certificate validation
		sc.dwFlags |= SCH_CRED_NO_SERVERNAME_CHECK | SCH_CRED_MANUAL_CRED_VALIDATION;

	} else {
		sc.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
		ctx->flags |= ISC_REQ_STREAM;
	}

	// Set self signed client cert if provided
	if (cert) {
		sc.cCreds = 1;
		sc.paCred = &cert->cert;
	}

	SECURITY_STATUS e = AcquireCredentialsHandle(NULL, TLS_PROVIDER, SECPKG_CRED_OUTBOUND,
		NULL, &sc, NULL, NULL, &ctx->ch, NULL);

	if (e != SEC_E_OK) {
		MTY_Log("'AcquireCredentialsHandle' failed with error 0x%X", e);
		tls_engine_destroy(&ctx);
	}

	return ctx;
}

static MTY_Async tls_engine_handshake_init(struct tls_engine *ctx, MTY_DTLSWriteFunc func, void *opaque)
{
	MTY_Async r = MTY_ASYNC_CONTINUE;

	unsigned long out_flags = 0;

	// Setup the output buffer, dynamically allocated by schannel
	SecBuffer out = {0};
	out.BufferType = SECBUFFER_EMPTY;

	SecBufferDesc desc = {0};
	desc.ulVersion = SECBUFFER_VERSION;
	desc.pBuffers = &out;
	desc.cBuffers = 1;

	// Client only -- generates the Client Hello and initializes the context
	SECURITY_STATUS e = InitializeSecurityContext(&ctx->ch, NULL, ctx->host, ctx->flags,
		0, 0, NULL, 0, &ctx->ctx, &desc, &out_flags, NULL);

	if (e != SEC_I_CONTINUE_NEEDED) {
		MTY_Log("'InitializeSecurityContext' failed with error 0x%X", e);
		r = MTY_ASYNC_ERROR;
		goto except;
	}

	// Set the MTU if DTLS
	if (ctx->dtls) {
		e = SetContextAttributes(&ctx->ctx, SECPKG_ATTR_DTLS_MTU, &ctx->mtu, sizeof(ULONG));
		if (e != SEC_E_OK) {
			MTY_Log("'SetContextAttributes' failed with error 0x%X", e);
			r = MTY_ASYNC_ERROR;
			goto except;
		}
	}

	// Write the client hello via the callback
	if (!func(out.pvBuffer, out.cbBuffer, opaque)) {
		r = MTY_ASYNC_ERROR;
		goto except;
	}

	except:

	if (out.pvBuffer)
		FreeContextBuffer(out.pvBuffer);

	if (r == MTY_ASYNC_CONTINUE)
		ctx->ctx_init = true;

	return r;
}

static bool tls_engine_verify_peer_fingerprint(struct tls_engine *ctx, const char *fingerprint)
{
	CERT_CONTEXT *rcert = NULL;
	SECURITY_STATUS e = QueryContextAttributes(&ctx->ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &rcert);
	if (e != SEC_E_OK) {
		MTY_Log("'QueryContextAttributes' failed with error 0x%X", e);
		return false;
	}

	if (rcert) {
		char found[MTY_FINGERPRINT_MAX];
		tls_engine_cert_context_to_fingerprint(rcert, found, MTY_FINGERPRINT_MAX);

		bool match = !strcmp(found, fingerprint);

		CertFreeCertificateContext(rcert);

		return match;
	}

	return false;
}

MTY_Async tls_engine_handshake(struct tls_engine *ctx, const void *buf, size_t size,
	MTY_DTLSWriteFunc func, void *opaque)
{
	if (!buf || size == 0)
		return !ctx->ctx_init ? tls_engine_handshake_init(ctx, func, opaque) : MTY_ASYNC_CONTINUE;

	MTY_Async r = MTY_ASYNC_OK;

	unsigned long out_flags = 0;

	// Input buffer contains TLS messages from the server
	SecBuffer in[2] = {0};
	in[0].BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
	in[0].pvBuffer = (void *) buf;
	in[0].cbBuffer = (unsigned long) size;

	in[1].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc in_desc = {0};
	in_desc.ulVersion = SECBUFFER_VERSION;
	in_desc.pBuffers = in;
	in_desc.cBuffers = 2;

	// Output buffer constains TLS handshake messages to be sent to the server
	SecBuffer out[3] = {0};
	out[0].BufferType = SECBUFFER_TOKEN;
	out[1].BufferType = SECBUFFER_ALERT;
	out[2].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc out_desc = {0};
	out_desc.ulVersion = SECBUFFER_VERSION;
	out_desc.pBuffers = out;
	out_desc.cBuffers = 3;

	// Feed the TLS handshake message to schannel
	SECURITY_STATUS e = InitializeSecurityContext(&ctx->ch, &ctx->ctx, ctx->host, ctx->flags,
		0, 0, &in_desc, 0, NULL, &out_desc, &out_flags, NULL);

	// Call write func if we have output data
	if (e == SEC_E_OK || e == SEC_I_CONTINUE_NEEDED || e == SEC_I_MESSAGE_FRAGMENT) {
		bool write_ok = true;

		if (out[0].pvBuffer && out[0].cbBuffer > 0)
			write_ok = func(out[0].pvBuffer, out[0].cbBuffer, opaque);

		r = !write_ok ? MTY_ASYNC_ERROR : e == SEC_E_OK ? MTY_ASYNC_OK : MTY_ASYNC_CONTINUE;

		// Validate peer certificate fingerprint if supplied
		if (ctx->fp && r == MTY_ASYNC_OK)
			r = tls_engine_verify_peer_fingerprint(ctx, ctx->fp) ? MTY_ASYNC_OK : MTY_ASYNC_ERROR;

	// Fatal error
	} else {
		MTY_Log("'InitializeSecurityContext' failed with error 0x%X", e);
		r = MTY_ASYNC_ERROR;
	}

	// Free dynamically allocated buffers from schannel
	for (uint8_t x = 0; x < 3; x++)
		if (out[x].pvBuffer && out[x].cbBuffer > 0)
			FreeContextBuffer(out[x].pvBuffer);

	return r;
}

bool tls_engine_encrypt(struct tls_engine *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	// Query sizes
	SecPkgContext_StreamSizes sizes = {0};
	SECURITY_STATUS e = QueryContextAttributes(&ctx->ctx, SECPKG_ATTR_STREAM_SIZES, &sizes);
	if (e != SEC_E_OK) {
		MTY_Log("'QueryContextAttributes' failed with error 0x%X", e);
		return false;
	}

	// Max message check
	if (inSize > sizes.cbMaximumMessage) {
		MTY_Log("TLS message size is too large (%zu > %u)", inSize, sizes.cbMaximumMessage);
		return false;
	}

	// Bounds check
	size_t full_size = sizes.cbHeader + inSize + sizes.cbTrailer;
	if (full_size > outSize) {
		MTY_Log("Output buffer is too small (%zu < %zu)", outSize, full_size);
		return false;
	}

	// Set up buffers and encrypt
	SecBuffer bufs[4] = {0};
	bufs[0].BufferType = SECBUFFER_STREAM_HEADER;
	bufs[0].pvBuffer = out;
	bufs[0].cbBuffer = sizes.cbHeader;

	bufs[1].BufferType = SECBUFFER_DATA;
	bufs[1].pvBuffer = (uint8_t *) out + sizes.cbHeader;
	bufs[1].cbBuffer = (unsigned long) inSize;

	bufs[2].BufferType = SECBUFFER_STREAM_TRAILER;
	bufs[2].pvBuffer = (uint8_t *) out + sizes.cbHeader + inSize;;
	bufs[2].cbBuffer = sizes.cbTrailer;

	bufs[3].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc desc = {0};
	desc.ulVersion = SECBUFFER_VERSION;
	desc.pBuffers = bufs;
	desc.cBuffers = 4;

	// Carefully protect agains overlapping in and out buffers
	memmove(bufs[1].pvBuffer, in, inSize);
	memset(bufs[0].pvBuffer, 0, sizes.cbHeader);
	memset(bufs[2].pvBuffer, 0, sizes.cbTrailer);

	e = EncryptMessage(&ctx->ctx, 0, &desc, 0);
	if (e != SEC_E_OK) {
		MTY_Log("'EncryptMessage' failed with error 0x%X", e);
		return false;
	}

	// The first two buffers remain contiguous, but the trailer size may change
	*written = bufs[0].cbBuffer + bufs[1].cbBuffer + bufs[2].cbBuffer;

	return true;
}

bool tls_engine_decrypt(struct tls_engine *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	// Bounds check
	if (outSize < inSize) {
		MTY_Log("Output buffer is too small (%zu < %zu)", outSize, inSize);
		return false;
	}

	// Prepare buffers and decrypt
	SecBuffer bufs[4] = {0};
	bufs[0].BufferType = SECBUFFER_DATA;
	bufs[0].pvBuffer = out;
	bufs[0].cbBuffer = (unsigned long) inSize;

	bufs[1].BufferType = SECBUFFER_EMPTY;
	bufs[2].BufferType = SECBUFFER_EMPTY;
	bufs[3].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc desc = {0};
	desc.ulVersion = SECBUFFER_VERSION;
	desc.pBuffers = bufs;
	desc.cBuffers = 4;

	// These buffers may overlap
	memmove(out, in, inSize);

	SECURITY_STATUS e = DecryptMessage(&ctx->ctx, &desc, 0, NULL);
	if (e != SEC_E_OK) {
		MTY_Log("'DecryptMessage' failed with error 0x%X", e);
		return false;
	}

	// Plain text resides in bufs[1]
	*read = bufs[1].cbBuffer;
	memmove(out, bufs[1].pvBuffer, *read);

	return true;
}
