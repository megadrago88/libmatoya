// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>

#define MTY_NET_PORT    80
#define MTY_NET_PORT_S  443

enum http_type {
	HTTP_INT,
	HTTP_STRING,
};

enum {
	MTY_NET_HTTP_OK               = 0,
	MTY_NET_HTTP_ERR_DEFAULT      = -1,
	MTY_NET_HTTP_ERR_PARSE_STATUS = -52000,
	MTY_NET_HTTP_ERR_PARSE_HEADER = -52001,
	MTY_NET_HTTP_ERR_PARSE_SCHEME = -52002,
	MTY_NET_HTTP_ERR_PARSE_HOST   = -52003,
	MTY_NET_HTTP_ERR_NOT_FOUND    = -52004,
};

struct http_header;

char *http_request(const char *method, const char *host, const char *path, const char *fields);
char *http_connect(const char *host, uint16_t port, const char *fields);
char *http_response(const char *code, const char *msg, const char *fields);
char *http_lc(char *str);
struct http_header *http_parse_header(char *header);
void http_free_header(struct http_header *h);
int32_t http_get_header(struct http_header *h, const char *key, int32_t *val_int, char **val_str);
int32_t http_get_status_code(struct http_header *h, uint16_t *status_code);
char *http_set_header(char *header, const char *name, int32_t type, const void *value);
int32_t http_parse_url(const char *url_in, int32_t *scheme, char **host, uint16_t *port, char **path);
