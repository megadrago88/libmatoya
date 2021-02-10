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

struct tcp_context;

int32_t tcp_error(void);
int32_t tcp_would_block(void);
int32_t tcp_in_progress(void);
int32_t tcp_bad_fd(void);

void tcp_close(struct tcp_context *nc);
int32_t tcp_poll(struct tcp_context *nc, int32_t tcp_event, int32_t timeout_ms);
int32_t tcp_getip4(const char *host, char *ip4, uint32_t ip4_len);
int32_t tcp_connect(struct tcp_context **nc_out, const char *ip4, uint16_t port, int32_t timeout_ms);
int32_t tcp_listen(struct tcp_context **nc_out, const char *bind_ip4, uint16_t port);
int32_t tcp_accept(struct tcp_context *nc, struct tcp_context **child, int32_t timeout_ms);
void tcp_get_socket(struct tcp_context *nc, void *socket);

int32_t tcp_write(void *ctx, const char *buf, size_t size);
int32_t tcp_read(void *ctx, char *buf, size_t size, int32_t timeout_ms);
