// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "tcp.h"

enum {
	MTY_NET_TLS_OK                = 0,
	MTY_NET_TLS_ERR_CONTEXT       = -51000,
	MTY_NET_TLS_ERR_SSL           = -51001,
	MTY_NET_TLS_ERR_FD            = -51002,
	MTY_NET_TLS_ERR_HANDSHAKE     = -51003,
	MTY_NET_TLS_ERR_WRITE         = -51004,
	MTY_NET_TLS_ERR_READ          = -51005,
	MTY_NET_TLS_ERR_CLOSED        = -51006,
	MTY_NET_TLS_ERR_CACERT        = -51007,
	MTY_NET_TLS_ERR_CIPHER        = -51008,
	MTY_NET_TLS_ERR_CERT          = -51009,
	MTY_NET_TLS_ERR_KEY           = -51010,
};

struct tls_context;

bool tls_load_cacert(const char *cacert, size_t size);

void tls_close(struct tls_context *tls);
int32_t tls_connect(struct tls_context **tls_in, TCP_SOCKET socket,
	const char *host, bool verify_host, int32_t timeout_ms);
int32_t tls_accept(struct tls_context **tls_in, TCP_SOCKET socket, int32_t timeout_ms);

int32_t tls_write(void *ctx, const char *buf, size_t size);
int32_t tls_read(void *ctx, char *buf, size_t size, int32_t timeout_ms);
