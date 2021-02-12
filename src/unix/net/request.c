// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <string.h>

#include "net/net.h"
#include "net/gzip.h"

static void mty_http_set_headers(struct mty_net_conn *ucc, char *header_str_orig)
{
	char *tok, *ptr = NULL;
	char *header_str = MTY_Strdup(header_str_orig);

	tok = MTY_Strtok(header_str, "\n", &ptr);
	while (tok) {
		char *key, *val = NULL, *ptr2 = NULL;

		key = MTY_Strtok(tok, ":", &ptr2);
		if (key) val = MTY_Strtok(NULL, ":", &ptr2);

		if (key && val) {
			mty_net_set_header_str(ucc, key, val);
		} else break;

		tok = MTY_Strtok(NULL, "\n", &ptr);
	}

	free(header_str);
}

int32_t mty_http_request(char *method, enum mty_net_scheme uc_scheme,
	char *host, char *path, char *headers, char *body, uint32_t body_len, int32_t timeout_ms,
	char **response, uint32_t *response_len, bool proxy)
{
	*response_len = 0;
	*response = NULL;

	int32_t z_e = MTY_NET_OK;
	int32_t r = MTY_NET_OK;
	int32_t status_code = 0;
	uint16_t port = (uc_scheme == MTY_NET_HTTPS) ? 443 : 80;

	//make the socket/TLS connection
	struct mty_net_conn *ucc = mty_net_new_conn();

	//make the TCP connection
	int32_t e = mty_net_connect(ucc, uc_scheme, host, port, true, NULL, 0, timeout_ms);
	if (e != MTY_NET_OK) goto except;

	//set request headers
	mty_net_set_header_str(ucc, "User-Agent", "mty_net/0.0");
	mty_net_set_header_str(ucc, "Connection", "close");
	if (headers) mty_http_set_headers(ucc, headers);
	if (body_len) mty_net_set_header_int(ucc, "Content-Length", body_len);

	//send the request header
	e = mty_net_write_header(ucc, method, path, MTY_NET_REQUEST);
	if (e != MTY_NET_OK) goto except;

	//send the request body
	e = mty_net_write_body(ucc, body, body_len);
	if (e != MTY_NET_OK) goto except;

	//read the response header
	e = mty_net_read_header(ucc, timeout_ms);
	if (e != MTY_NET_OK) goto except;

	//get the status code
	e = mty_net_get_status_code(ucc, &status_code);
	if (e != MTY_NET_OK) goto except;

	//read the response body if not HEAD request -- uncompress if necessary
	if (strcmp(method, "HEAD")) {
		e = mty_net_read_body_all(ucc, response, response_len, timeout_ms, 128 * 1024 * 1024);

		if (e == MTY_NET_OK && mty_net_check_header(ucc, "Content-Encoding", "gzip")) {
			size_t response_len_z = 0;
			char *response_z = mty_gzip_decompress(*response, *response_len, &response_len_z);

			if (response_z) {
				free(*response);

				*response = response_z;
				*response_len = response_len_z;

				z_e = MTY_NET_OK;

			} else {
				z_e = MTY_NET_ERR_INFLATE;
			}
		}
	}

	except:

	mty_net_close(ucc);

	if ((e != MTY_NET_OK && e != MTY_NET_ERR_NO_BODY) || z_e != MTY_NET_OK) {
		free(*response);
		*response_len = 0;
		*response = NULL;
		r = MTY_NET_HTTP_ERR_REQUEST;
	}

	return (r == MTY_NET_OK) ? status_code : r;
}
