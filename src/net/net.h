// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct mty_net_conn;

enum mty_net_header_type {
	MTY_NET_REQUEST  = 0,
	MTY_NET_RESPONSE = 1,
};

enum mty_net_status {
	MTY_NET_OK                    = 0,

	MTY_NET_WRN_CONTINUE          = 10,
	MTY_NET_WRN_TIMEOUT           = 2000,

	MTY_NET_ERR_DEFAULT           = -50001,

	MTY_NET_ERR_NO_BODY           = -53000,
	MTY_NET_ERR_MAX_CHUNK         = -53001,
	MTY_NET_ERR_MAX_BODY          = -53002,
	MTY_NET_ERR_MAX_HEADER        = -53003,
	MTY_NET_ERR_BUFFER            = -53004,
	MTY_NET_ERR_PROXY             = -53005,

	MTY_NET_WS_ERR_STATUS         = -54000,
	MTY_NET_WS_ERR_KEY            = -54001,
	MTY_NET_WS_ERR_ORIGIN         = -54002,

	MTY_NET_WS_ERR_POLL           = -3001,
	MTY_NET_WS_ERR_PING           = -3005,
	MTY_NET_WS_ERR_PONG_TIMEOUT   = -3006,
	MTY_NET_WS_ERR_PONG           = -3007,

	MTY_NET_ERR_INFLATE           = -5001,
};

char *mty_net_get_proxy(void);
struct mty_net_conn *mty_net_new_conn(void);
void mty_net_close(struct mty_net_conn *ucc);
void mty_net_set_header_str(struct mty_net_conn *ucc, const char *name, const char *value);
int32_t mty_net_read_header(struct mty_net_conn *ucc, int32_t timeout_ms);
int32_t mty_net_write_body(struct mty_net_conn *ucc, const char *body, uint32_t body_len);
int8_t mty_net_check_header(struct mty_net_conn *ucc, const char *name, const char *subval);
void mty_net_set_header_int(struct mty_net_conn *ucc, const char *name, int32_t value);
int32_t mty_net_read_body_all(struct mty_net_conn *ucc, void **body, size_t *body_len,
	int32_t timeout_ms, size_t max_body);
int32_t mty_net_write_header(struct mty_net_conn *ucc, const char *str0, const char *str1, int32_t type);
int32_t mty_net_get_status_code(struct mty_net_conn *ucc, uint16_t *status_code);
int32_t mty_net_connect(struct mty_net_conn *ucc, bool secure, const char *host, uint16_t port,
	bool verify_host, int32_t timeout_ms);
void mty_http_set_headers(struct mty_net_conn *ucc, const char *header_str_orig);
