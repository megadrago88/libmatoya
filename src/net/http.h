// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "net.h"

#define MTY_NET_PORT   80
#define MTY_NET_PORT_S 443

struct http_header;

char *http_request(const char *method, const char *host, const char *path, const char *fields);
char *http_connect(const char *host, uint16_t port, const char *fields);
char *http_response(const char *code, const char *msg, const char *fields);

struct http_header *http_parse_header(const char *header);
void http_header_destroy(struct http_header **header);
bool http_get_status_code(struct http_header *h, uint16_t *status_code);
bool http_get_header_int(struct http_header *h, const char *key, int32_t *val);
bool http_get_header_str(struct http_header *h, const char *key, const char **val);
char *http_set_header_int(char *header, const char *name, int32_t val);
char *http_set_header_str(char *header, const char *name, const char *val);
char *http_set_all_headers(char *header, const char *all);

struct http_header *http_read_header(struct mty_net *net, uint32_t timeout);
bool http_write_response_header(struct mty_net *net, const char *code, const char *reason, const char *headers);
bool http_write_request_header(struct mty_net *net, const char *method, const char *path, const char *headers);

char *http_get_proxy(void);
bool http_should_proxy(const char **host, uint16_t *port);
bool http_proxy_connect(struct mty_net *net, uint16_t port, uint32_t timeout);

bool http_parse_url(const char *url, bool *secure, char **host, uint16_t *port, char **path);
