// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct mty_net_conn;

struct mty_net_conn *mty_net_new_conn(void);
void mty_net_close(struct mty_net_conn *ucc);
void mty_net_set_header_str(struct mty_net_conn *ucc, const char *name, const char *value);
int32_t mty_net_read_header(struct mty_net_conn *ucc, int32_t timeout_ms);
int32_t mty_net_write_body(struct mty_net_conn *ucc, const char *body, uint32_t body_len);
int8_t mty_net_check_header(struct mty_net_conn *ucc, const char *name, const char *subval);
void mty_net_set_header_int(struct mty_net_conn *ucc, const char *name, int32_t value);
int32_t mty_net_read_body_all(struct mty_net_conn *ucc, char **body, uint32_t *body_len,
	int32_t timeout_ms, size_t max_body);
int32_t mty_net_write_header(struct mty_net_conn *ucc, const char *str0, const char *str1, int32_t type);
int32_t mty_net_get_status_code(struct mty_net_conn *ucc, int32_t *status_code);
int32_t mty_net_connect(struct mty_net_tls_ctx *uc_tls, struct mty_net_conn *ucc,
	int32_t scheme, const char *host, uint16_t port, bool verify_host,
	const char *proxy_host, uint16_t proxy_port, int32_t timeout_ms);
