// Copyright (c) 2021 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "matoya.h"

struct tls_engine;
struct tls_cert;

struct tls_cert *tls_engine_cert_create(void);
void tls_engine_cert_destroy(struct tls_cert **cert);
void tls_engine_cert_get_fingerprint(struct tls_cert *cert, char *fingerprint, size_t size);

struct tls_engine *tls_engine_create(bool dtls, const char *host, const char *verify_fingerprint,
	struct tls_cert *cert, uint32_t mtu);
void tls_engine_destroy(struct tls_engine **engine);

MTY_Async tls_engine_handshake(struct tls_engine *ctx, const void *buf, size_t size,
	MTY_DTLSWriteFunc func, void *opaque);

bool tls_engine_encrypt(struct tls_engine *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written);
bool tls_engine_decrypt(struct tls_engine *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read);
