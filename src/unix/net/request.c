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
	req = http_set_header_str(req, "User-Agent", "mty-http/v4");
	req = http_set_header_str(req, "Connection", "close");

	if (headers)
		req = http_set_all_headers(req, headers);

	if (bodySize)
		req = http_set_header_int(req, "Content-Length", bodySize);

	// Send the request header
	r = http_write_request_header(net, method, path, req);
	if (!r)
		goto except;

	// Send the request body
	r = mty_net_write(net, body, bodySize);
	if (!r)
		goto except;

	// Read the response header
	hdr = http_read_header(net, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	// Get the status code
	r = http_get_status_code(hdr, status);
	if (!r)
		goto except;

	// If response has a body, read it and uncompress it
	if (http_get_header_int(hdr, "Content-Length", (int32_t *) responseSize) && *responseSize > 0) {
		*response = MTY_Alloc(*responseSize + 1, 1);

		r = mty_net_read(net, *response, *responseSize, timeout);
		if (!r)
			goto except;

		// Check for content-encoding header and attempt to uncompress
		const char *val = NULL;
		if (http_get_header_str(hdr, "Content-Encoding", &val) && !MTY_Strcasecmp(val, "gzip")) {
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
	http_header_destroy(&hdr);
	mty_net_destroy(&net);

	if (!r) {
		MTY_Free(*response);
		*responseSize = 0;
		*response = NULL;
	}

	return r;
}
