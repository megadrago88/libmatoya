// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <winhttp.h>

#include "net/http.h"
#include "net/gzip.h"

bool MTY_HttpRequest(const char *_host, bool secure, const char *_method, const char *_path,
	const char *_headers, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status)
{
	*responseSize = 0;
	*response = NULL;

	bool r = true;
	HINTERNET session = NULL;
	HINTERNET connect = NULL;
	HINTERNET request = NULL;

	// WCHAR conversion
	WCHAR host[512];
	_snwprintf_s(host, 512, _TRUNCATE, L"%hs", _host);

	WCHAR path[512];
	_snwprintf_s(path, 512, _TRUNCATE, L"%hs", _path);

	WCHAR method[32];
	_snwprintf_s(method, 32, _TRUNCATE, L"%hs", _method);

	WCHAR headers[512];
	if (_headers)
		_snwprintf_s(headers, 512, _TRUNCATE, L"%hs", _headers);

	// Proxy
	const char *proxy = http_get_proxy();
	DWORD access_type = WINHTTP_ACCESS_TYPE_NO_PROXY;
	WCHAR *wproxy = WINHTTP_NO_PROXY_NAME;
	WCHAR wproxy_buf[512];

	if (proxy) {
		access_type = WINHTTP_ACCESS_TYPE_NAMED_PROXY;

		_snwprintf_s(wproxy_buf, 512, _TRUNCATE, L"%hs", proxy);
		wproxy = wproxy_buf;
	}

	// Context initialization
	session = WinHttpOpen(L"mty-winhttp/v4", access_type, wproxy, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) {
		r = false;
		goto except;
	}

	// Set timeouts
	r = WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);
	if (!r)
		goto except;

	// Attempt to force TLS 1.2, ignore failure
	DWORD opt = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &opt, sizeof(DWORD));

	connect = WinHttpConnect(session, host, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!connect) {
		r = false;
		goto except;
	}

	// HTTP TCP/TLS connection
	request = WinHttpOpenRequest(connect, method, path, NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
	if (!request) {
		r = false;
		goto except;
	}

	// Write headers and body
	r = WinHttpSendRequest(request, _headers ? headers : WINHTTP_NO_ADDITIONAL_HEADERS,
		_headers ? -1L : 0, (void *) body, (DWORD) bodySize, (DWORD) bodySize, 0);
	if (!r)
		goto except;

	// Read response headers
	r = WinHttpReceiveResponse(request, NULL);
	if (!r)
		goto except;

	// Status code query
	WCHAR header_wstr[128];
	DWORD buf_len = 128 * sizeof(WCHAR);
	r = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, NULL, header_wstr, &buf_len, NULL);
	if (!r)
		goto except;

	*status = (uint16_t) _wtoi(header_wstr);

	// Content encoding query
	buf_len = 128 * sizeof(WCHAR);
	bool gzipped = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_ENCODING, NULL, header_wstr, &buf_len, NULL) &&
		!wcscmp(header_wstr, L"gzip");

	// Receive response body
	while (true) {
		DWORD available = 0;
		r = WinHttpQueryDataAvailable(request, &available);
		if (!r)
			goto except;

		if (available == 0)
			break;

		*response = MTY_Realloc(*response, *responseSize + available + 1, 1);

		DWORD read = 0;
		r = WinHttpReadData(request, (uint8_t *) *response + *responseSize, available, &read);
		if (!r)
			goto except;

		*responseSize += read;

		// Keep null character at the end of the buffer for protection
		memset((uint8_t *) *response + *responseSize, 0, 1);
	}

	// Optionally uncompress
	if (gzipped && *response && *responseSize > 0) {
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

	except:

	if (request)
		WinHttpCloseHandle(request);

	if (connect)
		WinHttpCloseHandle(connect);

	if (session)
		WinHttpCloseHandle(session);

	if (!r) {
		MTY_Free(*response);
		*responseSize = 0;
		*response = NULL;
	}

	return r;
}
