// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct mty_net;

enum mty_net_header_type {
	MTY_NET_REQUEST  = 0,
	MTY_NET_RESPONSE = 1,
};

enum mty_net_status {
	MTY_NET_OK                    = 0,
	MTY_NET_ERR_DEFAULT           = -50001,
	MTY_NET_ERR_NO_BODY           = -53000,
	MTY_NET_ERR_MAX_CHUNK         = -53001,
	MTY_NET_ERR_MAX_BODY          = -53002,
	MTY_NET_ERR_MAX_HEADER        = -53003,
	MTY_NET_ERR_BUFFER            = -53004,
	MTY_NET_WS_ERR_STATUS         = -54000,
	MTY_NET_WS_ERR_KEY            = -54001,
	MTY_NET_WS_ERR_ORIGIN         = -54002,
	MTY_NET_WS_ERR_PING           = -3005,
	MTY_NET_ERR_INFLATE           = -5001,
};

char *mty_net_get_proxy(void);

struct mty_net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout);
struct mty_net *mty_net_listen(const char *ip, uint16_t port);
struct mty_net *mty_net_accept(struct mty_net *ctx, bool secure, uint32_t timeout);
void mty_net_destroy(struct mty_net **net);

bool mty_net_write(struct mty_net *ucc, const void *buf, size_t size);
bool mty_net_read(struct mty_net *ucc, void *buf, size_t size, uint32_t timeout);
MTY_Async mty_net_poll(struct mty_net *ucc, uint32_t timeout);

void mty_net_set_header_str(struct mty_net *ucc, const char *name, const char *value);
int32_t mty_net_read_header(struct mty_net *ucc, int32_t timeout);
void mty_net_set_header_int(struct mty_net *ucc, const char *name, int32_t value);
int32_t mty_net_write_header(struct mty_net *ucc, const char *str0, const char *str1, int32_t type);
bool mty_net_get_status_code(struct mty_net *ucc, uint16_t *status_code);
bool mty_net_get_header_str(struct mty_net *ucc, const char *key, const char **val);
bool mty_net_get_header_int(struct mty_net *ucc, const char *key, int32_t *val);
void mty_http_set_headers(struct mty_net *ucc, const char *header_str_orig);
