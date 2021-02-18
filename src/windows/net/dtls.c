// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct MTY_Cert {
	bool dummy;
};

struct MTY_DTLS {
	bool dummy;
};


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	return NULL;
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	MTY_Free(ctx);
	*cert = NULL;
}


// DTLS

MTY_DTLS *MTY_DTLSCreate(MTY_Cert *cert, bool server, uint32_t mtu)
{
	return NULL;
}

void MTY_DTLSDestroy(MTY_DTLS **dtls)
{
	if (!dtls || !*dtls)
		return;

	MTY_DTLS *ctx = *dtls;

	MTY_Free(ctx);
	*dtls = NULL;
}

MTY_Async MTY_DTLSHandshake(MTY_DTLS *ctx, const void *packet, size_t size, const char *fingerprint,
	MTY_DTLSWriteFunc writeFunc, void *opaque)
{
	return MTY_ASYNC_ERROR;
}

bool MTY_DTLSEncrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	return false;
}

bool MTY_DTLSDecrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	return false;
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
