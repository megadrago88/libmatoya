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

struct tls;

bool tls_load_cacert(const char *cacert, size_t size);

struct tls *tls_connect(TCP_SOCKET socket, const char *host, bool verify_host, int32_t timeout_ms);
struct tls *tls_accept(TCP_SOCKET socket, int32_t timeout_ms);
void tls_destroy(struct tls **tls);

int32_t tls_write(void *ctx, const char *buf, size_t size);
int32_t tls_read(void *ctx, char *buf, size_t size, int32_t timeout_ms);
