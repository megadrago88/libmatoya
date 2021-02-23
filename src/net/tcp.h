// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "matoya.h"

intptr_t tcp_connect(const char *ip, uint16_t port, uint32_t timeout);
intptr_t tcp_listen(const char *ip, uint16_t port);
intptr_t tcp_accept(intptr_t s, uint32_t timeout);
void tcp_destroy(intptr_t *socket);

MTY_Async tcp_poll(intptr_t s, bool out, uint32_t timeout);
bool tcp_write(intptr_t s, const void *buf, size_t size);
bool tcp_read(intptr_t s, void *buf, size_t size, uint32_t timeout);

bool dns_query(const char *host, char *ip, size_t size);
