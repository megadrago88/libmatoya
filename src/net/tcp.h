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

// This is OK for both winsock and unix sockets
#define TCP_INVALID_SOCKET (-1)

typedef intptr_t TCP_SOCKET;

TCP_SOCKET tcp_connect(const char *ip, uint16_t port, uint32_t timeout);
TCP_SOCKET tcp_listen(const char *ip, uint16_t port);
TCP_SOCKET tcp_accept(TCP_SOCKET s, uint32_t timeout);
void tcp_destroy(TCP_SOCKET *socket);

MTY_Async tcp_async(void);
MTY_Async tcp_poll(TCP_SOCKET s, bool out, uint32_t timeout);
bool tcp_write(TCP_SOCKET s, const void *buf, size_t size);
bool tcp_read(TCP_SOCKET s, void *buf, size_t size, uint32_t timeout);

bool dns_query(const char *host, char *ip, size_t size);
