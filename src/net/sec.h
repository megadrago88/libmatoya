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

struct tls *tls_connect(TCP_SOCKET socket, const char *host, uint32_t timeout);
struct tls *tls_accept(TCP_SOCKET socket, uint32_t timeout);
void tls_destroy(struct tls **tls);

bool tls_write(struct tls *ctx, const void *buf, size_t size);
bool tls_read(struct tls *ctx, void *buf, size_t size, uint32_t timeout);
