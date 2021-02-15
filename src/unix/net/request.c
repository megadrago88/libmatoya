// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <string.h>

#include "net/net.h"
#include "net/http.h"
#include "net/gzip.h"

bool MTY_HttpRequest(const char *method, const char *headers, bool secure,
	const char *host, const char *path, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status)
{
	*responseSize = 0;
	*response = NULL;

	int32_t z_e = MTY_NET_OK;
	bool ok = true;
	uint16_t port = secure ? MTY_NET_PORT_S : MTY_NET_PORT;

	//make the socket/TLS connection
	struct mty_net_conn *ucc = mty_net_new_conn();

	//make the TCP connection
	int32_t e = mty_net_connect(ucc, secure, host, port, true, timeout);
	if (e != MTY_NET_OK) goto except;

	//set request headers
	mty_net_set_header_str(ucc, "User-Agent", "mty_net/0.0");
	mty_net_set_header_str(ucc, "Connection", "close");
	if (headers) mty_http_set_headers(ucc, headers);
	if (bodySize) mty_net_set_header_int(ucc, "Content-Length", bodySize);

	//send the request header
	e = mty_net_write_header(ucc, method, path, MTY_NET_REQUEST);
	if (e != MTY_NET_OK) goto except;

	//send the request body
	e = mty_net_write_body(ucc, body, bodySize);
	if (e != MTY_NET_OK) goto except;

	//read the response header
	e = mty_net_read_header(ucc, timeout);
	if (e != MTY_NET_OK) goto except;

	//get the status code
	e = mty_net_get_status_code(ucc, status);
	if (e != MTY_NET_OK) goto except;

	//read the response body if not HEAD request -- uncompress if necessary
	if (strcmp(method, "HEAD")) {
		e = mty_net_read_body_all(ucc, response, responseSize, timeout, 128 * 1024 * 1024);

		if (e == MTY_NET_OK && mty_net_check_header(ucc, "Content-Encoding", "gzip")) {
			size_t response_len_z = 0;
			char *response_z = mty_gzip_decompress(*response, *responseSize, &response_len_z);

			if (response_z) {
				free(*response);

				*response = response_z;
				*responseSize = response_len_z;

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
		*responseSize = 0;
		*response = NULL;
		ok = false;
	}

	return ok;
}
