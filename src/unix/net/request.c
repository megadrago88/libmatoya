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

static int8_t mty_net_check_header(struct mty_net_conn *ucc, const char *name, const char *subval)
{
	const char *val = NULL;
	bool ok = mty_net_get_header_str(ucc, name, &val);

	return ok && MTY_Strcasecmp(val, subval) ? 1 : 0;
}

static int32_t mty_net_read_chunk_len(struct mty_net_conn *ucc, uint32_t *len, int32_t timeout_ms)
{
	int32_t r = MTY_NET_ERR_MAX_CHUNK;

	char chunk_len[LEN_CHUNK];
	memset(chunk_len, 0, LEN_CHUNK);

	for (uint32_t x = 0; x < LEN_CHUNK - 1; x++) {
		if (!mty_net_read(ucc, chunk_len + x, 1, timeout_ms))
			{r = MTY_NET_ERR_DEFAULT; break;}

		if (x > 0 && chunk_len[x - 1] == '\r' && chunk_len[x] == '\n') {
			chunk_len[x - 1] = '\0';
			*len = strtoul(chunk_len, NULL, 16);
			return MTY_NET_OK;
		}
	}

	*len = 0;

	return r;
}

static int32_t mty_net_response_body_chunked(struct mty_net_conn *ucc, void **body, size_t *body_len,
	int32_t timeout_ms, size_t max_body)
{
	uint32_t offset = 0;
	uint32_t chunk_len = 0;

	do {
		//read the chunk size one byte at a time
		int32_t e = mty_net_read_chunk_len(ucc, &chunk_len, timeout_ms);
		if (e != MTY_NET_OK) return e;
		if (offset + chunk_len > max_body) return MTY_NET_ERR_MAX_BODY;

		//make room for chunk and "\r\n" after chunk
		*body = realloc(*body, offset + chunk_len + 2);

		//read chunk into buffer with extra 2 bytes for "\r\n"
		if (!mty_net_read(ucc, (char *) *body + offset, chunk_len + 2, timeout_ms))
			return MTY_NET_ERR_DEFAULT;

		offset += chunk_len;

	} while (chunk_len > 0);

	((uint8_t *) *body)[offset] = '\0';
	*body_len = offset;

	return MTY_NET_OK;
}

static int32_t mty_net_read_body_all(struct mty_net_conn *ucc, void **body, size_t *body_len,
	int32_t timeout_ms, size_t max_body)
{
	int32_t r = MTY_NET_OK;

	*body = NULL;
	*body_len = 0;

	//look for chunked response
	if (mty_net_check_header(ucc, "Transfer-Encoding", "chunked")) {
		int32_t e = mty_net_response_body_chunked(ucc, body, body_len, timeout_ms, max_body);
		if (e != MTY_NET_OK) {r = e; goto except;}

	//use Content-Length
	} else {
		if (!mty_net_get_header_int(ucc, "Content-Length", (int32_t *) body_len)) {
			r = MTY_NET_ERR_DEFAULT;
			goto except;
		}

		if (*body_len == 0) {r = MTY_NET_ERR_NO_BODY; goto except;}
		if (*body_len > max_body) {r = MTY_NET_ERR_MAX_BODY; goto except;}

		*body = calloc(*body_len + 1, 1);

		if (!mty_net_read(ucc, *body, *body_len, timeout_ms))
			{r = MTY_NET_ERR_DEFAULT; goto except;}
	}

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

	int32_t z_e = MTY_NET_OK;
	bool ok = true;
	uint16_t port = secure ? MTY_NET_PORT_S : MTY_NET_PORT;

	//make the socket/TLS connection
	struct mty_net_conn *ucc = mty_net_new_conn();

	//make the TCP connection
	int32_t e = mty_net_connect(ucc, secure, host, port, timeout);
	if (e != MTY_NET_OK) goto except;

	//set request headers
	mty_net_set_header_str(ucc, "User-Agent", "mty-http/v4");
	mty_net_set_header_str(ucc, "Connection", "close");
	if (headers) mty_http_set_headers(ucc, headers);
	if (bodySize) mty_net_set_header_int(ucc, "Content-Length", bodySize);

	//send the request header
	e = mty_net_write_header(ucc, method, path, MTY_NET_REQUEST);
	if (e != MTY_NET_OK) goto except;

	//send the request body
	if (!mty_net_write(ucc, body, bodySize))
		{e = MTY_NET_ERR_DEFAULT; goto except;}

	//read the response header
	e = mty_net_read_header(ucc, timeout);
	if (e != MTY_NET_OK) goto except;

	//get the status code
	if (!mty_net_get_status_code(ucc, status)) {
		 e = MTY_NET_ERR_DEFAULT;
		 goto except;
	}

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
