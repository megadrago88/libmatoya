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

static bool http_read_chunk_len(struct mty_net *net, uint32_t timeout, size_t *len)
{
	*len = 0;
	char len_buf[64] = {0};

	for (uint32_t x = 0; x < 64 - 1; x++) {
		if (!mty_net_read(net, len_buf + x, 1, timeout))
			break;

		if (x > 0 && len_buf[x - 1] == '\r' && len_buf[x] == '\n') {
			len_buf[x - 1] = '\0';
			*len = strtoul(len_buf, NULL, 16);

			return true;
		}
	}

	return false;
}

static bool http_read_chunked(struct mty_net *net, void **res, size_t *size, uint32_t timeout)
{
	size_t chunk_len = 0;

	do {
		// Read the chunk size one byte at a time
		if (!http_read_chunk_len(net, timeout, &chunk_len))
			return false;

		// Make room for chunk and "\r\n" after chunk
		*res = MTY_Realloc(*res, *size + chunk_len + 2, 1);

		// Read chunk into buffer with extra 2 bytes for "\r\n"
		if (!mty_net_read(net, (uint8_t *) *res + *size, chunk_len + 2, timeout))
			return false;

		*size += chunk_len;

		// Keep null character at the end of the buffer for protection
		memset((uint8_t *) *res + *size, 0, 1);

	} while (chunk_len > 0);

	return true;
}

bool MTY_HttpRequest(const char *host, bool secure, const char *method, const char *path,
	const char *headers, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status)
{
	*responseSize = 0;
	*response = NULL;

	bool r = true;
	char *req = NULL;
	struct http_header *hdr = NULL;

	// Make the TCP/TLS connection
	struct mty_net *net = mty_net_connect(host, secure ? MTY_NET_PORT_S : MTY_NET_PORT, secure, timeout);
	if (!net) {
		r = false;
		goto except;
	}

	// Set request headers
	req = mty_http_set_header_str(req, "User-Agent", "mty-http/v4");
	req = mty_http_set_header_str(req, "Connection", "close");

	if (headers)
		req = mty_http_set_all_headers(req, headers);

	if (bodySize)
		req = mty_http_set_header_int(req, "Content-Length", bodySize);

	// Send the request header
	r = mty_http_write_request_header(net, method, path, req);
	if (!r)
		goto except;

	// Send the request body
	if (body && bodySize > 0) {
		r = mty_net_write(net, body, bodySize);
		if (!r)
			goto except;
	}

	// Read the response header
	hdr = mty_http_read_header(net, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	// Get the status code
	r = mty_http_get_status_code(hdr, status);
	if (!r)
		goto except;

	// Read response body -- either fixed content length or chunked
	const char *val = NULL;
	if (mty_http_get_header_int(hdr, "Content-Length", (int32_t *) responseSize) && *responseSize > 0) {
		*response = MTY_Alloc(*responseSize + 1, 1);

		r = mty_net_read(net, *response, *responseSize, timeout);
		if (!r)
			goto except;

	} else if (mty_http_get_header_str(hdr, "Transfer-Encoding", &val) && !MTY_Strcasecmp(val, "chunked")) {
		r = http_read_chunked(net, response, responseSize, timeout);
		if (!r)
			goto except;
	}

	// Check for content-encoding header and attempt to uncompress
	if (*response && *responseSize > 0) {
		if (mty_http_get_header_str(hdr, "Content-Encoding", &val) && !MTY_Strcasecmp(val, "gzip")) {
			size_t zlen = 0;
			void *z = mty_gzip_decompress(*response, *responseSize, &zlen);
			if (!z) {
				r = false;
				goto except;
			}

			MTY_Free(*response);
			*response = z;
			*responseSize = zlen;
		}
	}

	except:

	MTY_Free(req);
	mty_http_header_destroy(&hdr);
	mty_net_destroy(&net);

	if (!r) {
		MTY_Free(*response);
		*responseSize = 0;
		*response = NULL;
	}

	return r;
}
