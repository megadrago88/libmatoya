// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sec-engine.h"

struct MTY_Cert {
	struct tls_cert *ecert;
};

struct MTY_DTLS {
	struct tls_engine *engine;
};


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	MTY_Cert *cert = MTY_Alloc(1, sizeof(MTY_Cert));
	cert->ecert = tls_engine_cert_create();

	return cert;
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	tls_engine_cert_get_fingerprint(ctx->ecert, fingerprint, size);
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	tls_engine_cert_destroy(&ctx->ecert);

	MTY_Free(ctx);
	*cert = NULL;
}


// DTLS

MTY_DTLS *MTY_DTLSCreate(MTY_Cert *cert, bool server, uint32_t mtu, const char *peerFingerprint)
{
	// TODO send shutdown message

	MTY_DTLS *ctx = MTY_Alloc(1, sizeof(MTY_DTLS));
	ctx->engine = tls_engine_create(true, NULL, peerFingerprint, cert ? cert->ecert : NULL, mtu);

	return ctx;
}

void MTY_DTLSDestroy(MTY_DTLS **dtls)
{
	if (!dtls || !*dtls)
		return;

	MTY_DTLS *ctx = *dtls;

	tls_engine_destroy(&ctx->engine);

	MTY_Free(ctx);
	*dtls = NULL;
}

MTY_Async MTY_DTLSHandshake(MTY_DTLS *ctx, const void *packet, size_t size, MTY_DTLSWriteFunc writeFunc, void *opaque)
{
	return tls_engine_handshake(ctx->engine, packet, size, writeFunc, opaque);
}

bool MTY_DTLSEncrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	return tls_engine_encrypt(ctx->engine, in, inSize, out, outSize, written);
}

bool MTY_DTLSDecrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	return tls_engine_decrypt(ctx->engine, in, inSize, out, outSize, read);
}

bool MTY_DTLSIsHandshake(const void *packet, size_t size)
{
	const uint8_t *d = packet;

	// 0x16feff == DTLS 1.2 Client Hello
	// 0x16fefd == DTLS 1.2 Server Hello, Client Key Exchange, New Session Ticket
	return size > 2 && (d[0] == 0x16 || d[0] == 0x14) && (d[1] == 0xfe && (d[2] == 0xfd || d[2] == 0xff));
}

bool MTY_DTLSIsApplicationData(const void *packet, size_t size)
{
	const uint8_t *d = packet;

	// 0x17fefd == DTLS 1.2 Application Data
	return size > 2 && d[0] == 0x17 && (d[1] == 0xfe && d[2] == 0xfd);
}
