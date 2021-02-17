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

#define LEN_CHUNK  64

static bool mty_net_check_header(struct http_header *hdr, const char *name, const char *subval)
{
	const char *val = NULL;
	bool ok = http_get_header_str(hdr, name, &val);

	return ok && !MTY_Strcasecmp(val, subval);
}

static int32_t mty_net_read_body_all(struct mty_net *ucc, struct http_header *hdr,
	void **body, size_t *body_len, int32_t timeout_ms, size_t max_body)
{
	int32_t r = MTY_NET_OK;

	*body = NULL;
	*body_len = 0;

	if (!http_get_header_int(hdr, "Content-Length", (int32_t *) body_len)) {
		r = MTY_NET_ERR_DEFAULT;
		goto except;
	}

	if (*body_len == 0)
		goto except;

	if (*body_len > max_body) {r = MTY_NET_ERR_DEFAULT; goto except;}

	*body = calloc(*body_len + 1, 1);

	if (!mty_net_read(ucc, *body, *body_len, timeout_ms))
		{r = MTY_NET_ERR_DEFAULT; goto except;}

	except:

	if (r != MTY_NET_OK) {
		free(*body);
		*body = NULL;
	}

	return r;
}

bool MTY_HttpRequest(const char *host, bool secure, const char *method, const char *path,
	const char *headers, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status)
{
	*responseSize = 0;
	*response = NULL;

	bool ok = true;
	int32_t e = MTY_NET_OK;
	uint16_t port = secure ? MTY_NET_PORT_S : MTY_NET_PORT;
	struct http_header *hdr = NULL;
	char *req = NULL;

	//make the TCP connection
	struct mty_net *ucc = mty_net_connect(host, port, secure, timeout);
	if (!ucc)
		goto except;

	//set request headers
	req = http_set_header_str(req, "User-Agent", "mty-http/v4");
	req = http_set_header_str(req, "Connection", "close");

	if (headers)
		req = http_set_all_headers(req, headers);

	if (bodySize)
		req = http_set_header_int(req, "Content-Length", bodySize);

	//send the request header
	if (!http_write_request_header(ucc, method, path, req)) {
		e = MTY_NET_ERR_DEFAULT;
		goto except;
	}

	//send the request body
	if (!mty_net_write(ucc, body, bodySize)) {
		e = MTY_NET_ERR_DEFAULT;
		goto except;
	}

	//read the response header
	hdr = http_read_header(ucc, timeout);
	if (!hdr) {
		e = MTY_NET_ERR_DEFAULT;
		goto except;
	}

	//get the status code
	if (!http_get_status_code(hdr, status)) {
		 e = MTY_NET_ERR_DEFAULT;
		 goto except;
	}

	//read the response body if not HEAD request -- uncompress if necessary
	if (strcmp(method, "HEAD")) {
		e = mty_net_read_body_all(ucc, hdr, response, responseSize, timeout, 128 * 1024 * 1024);

		if (e == MTY_NET_OK && mty_net_check_header(hdr, "Content-Encoding", "gzip")) {
			size_t response_len_z = 0;
			char *response_z = mty_gzip_decompress(*response, *responseSize, &response_len_z);

			if (response_z) {
				free(*response);

				*response = response_z;
				*responseSize = response_len_z;

				e = MTY_NET_OK;

			} else {
				e = MTY_NET_ERR_DEFAULT;
			}
		}
	}

	except:

	MTY_Free(req);
	http_header_destroy(&hdr);
	mty_net_destroy(&ucc);

	if (e != MTY_NET_OK) {
		free(*response);
		*responseSize = 0;
		*response = NULL;
		ok = false;
	}

	return ok;
}
