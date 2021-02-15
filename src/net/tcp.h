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

struct tcp_context;

int32_t tcp_error(void);
int32_t tcp_would_block(void);
int32_t tcp_in_progress(void);
int32_t tcp_bad_fd(void);

void tcp_close(struct tcp_context *nc);
int32_t tcp_poll(struct tcp_context *nc, int32_t tcp_event, int32_t timeout_ms);
int32_t tcp_connect(struct tcp_context **nc_out, const char *ip4, uint16_t port, int32_t timeout_ms);
int32_t tcp_listen(struct tcp_context **nc_out, const char *bind_ip4, uint16_t port);
int32_t tcp_accept(struct tcp_context *nc, struct tcp_context **child, int32_t timeout_ms);
void tcp_get_socket(struct tcp_context *nc, void *socket);

int32_t tcp_write(void *ctx, const char *buf, size_t size);
int32_t tcp_read(void *ctx, char *buf, size_t size, int32_t timeout_ms);

bool dns_query(const char *host, char *ip, size_t size);
