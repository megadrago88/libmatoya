// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>

enum tcp_events {
	TCP_POLLIN  = 0,
	TCP_POLLOUT = 1,
};

enum {
	MTY_NET_TCP_OK                = 0,
	MTY_NET_TCP_ERR_SOCKET        = -50010,
	MTY_NET_TCP_ERR_BLOCKMODE     = -50011,
	MTY_NET_TCP_ERR_CONNECT       = -50012,
	MTY_NET_TCP_ERR_CONNECT_FINAL = -50013,
	MTY_NET_TCP_ERR_WRITE         = -50014,
	MTY_NET_TCP_ERR_READ          = -50015,
	MTY_NET_TCP_ERR_CLOSED        = -50016,
	MTY_NET_TCP_ERR_RESOLVE       = -50017,
	MTY_NET_TCP_ERR_NTOP          = -50018,
	MTY_NET_TCP_ERR_TIMEOUT       = -50019,
	MTY_NET_TCP_ERR_POLL          = -50020,
	MTY_NET_TCP_ERR_BIND          = -50021,
	MTY_NET_TCP_ERR_LISTEN        = -50022,
	MTY_NET_TCP_ERR_ACCEPT        = -50023,
};

typedef intptr_t TCP_SOCKET;

int32_t tcp_error(void);
int32_t tcp_would_block(void);
int32_t tcp_invalid_socket(void);
int32_t tcp_bad_fd(void);

TCP_SOCKET tcp_connect(const char *ip4, uint16_t port, int32_t timeout_ms);
TCP_SOCKET tcp_listen(const char *bind_ip4, uint16_t port);
TCP_SOCKET tcp_accept(TCP_SOCKET nc, int32_t timeout_ms);
int32_t tcp_poll(TCP_SOCKET nc, int32_t tcp_event, int32_t timeout_ms);
void tcp_destroy(TCP_SOCKET *nc);

int32_t tcp_write(TCP_SOCKET s, const char *buf, size_t size);
int32_t tcp_read(TCP_SOCKET s, char *buf, size_t size, int32_t timeout_ms);

bool dns_query(const char *host, char *ip, size_t size);
