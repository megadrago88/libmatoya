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

#include "net/gzip.h"

static int32_t mty_http_recv_response(HINTERNET request, char **response, uint32_t *response_len)
{
	DWORD available = 0;

	do {
		BOOL success = WinHttpQueryDataAvailable(request, &available);
		if (!success) {
			MTY_Log("'WinHttpQueryDataAvailable' failed with error 0x%X", GetLastError());
			return MTY_NET_HTTP_ERR_RESPONSE;
		}

		char *buf = malloc(available);

		DWORD read = 0;
		success = WinHttpReadData(request, buf, available, &read);

		if (success && read > 0) {
			*response = realloc(*response, *response_len + read + 1);
			memcpy(*response + *response_len, buf, read);
			*response_len += read;
		}

		free(buf);

		if (!success) {
			MTY_Log("'WinHttpReadData' failed with error 0x%X", GetLastError());
			return MTY_NET_HTTP_ERR_RESPONSE;
		}

	} while (available > 0);

	return MTY_NET_OK;
}

static int32_t mty_http_decompress(char **response, uint32_t *response_len)
{
	size_t response_len_z = 0;

	char *response_z = mty_gzip_decompress(*response, *response_len, &response_len_z);

	if (response_z) {
		free(*response);

		*response = response_z;
		*response_len = (uint32_t) response_len_z;

		return MTY_NET_OK;
	}

	return MTY_NET_ERR_INFLATE;
}

int32_t mty_http_request(const char *method, enum mty_net_scheme scheme,
	const char *host, const char *path, const char *headers, const void *body,
	uint32_t body_len, int32_t timeout_ms, char **response, uint32_t *response_len, bool proxy)
{
	HINTERNET session = NULL, connect = NULL, request = NULL;

	*response_len = 0;
	*response = NULL;

	int32_t r = MTY_NET_OK;
	int32_t status_code = 0;
	bool gzipped = false;

	//context initialization
	session = WinHttpOpen(L"mty-winhttp/v4", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) {
		MTY_Log("'WinHttpOpen' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_REQUEST;
		goto except;
	}

	//set timeouts
	BOOL success = WinHttpSetTimeouts(session, timeout_ms, timeout_ms, timeout_ms, timeout_ms);
	if (!success) {
		MTY_Log("'WinHttpSetTimeouts' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_REQUEST;
		goto except;
	}

	//tcp connection
	WCHAR host_w[512];
	_snwprintf_s(host_w, 512, _TRUNCATE, L"%hs", host);

	//attempt to force TLS 1.2, ignore failure
	DWORD opt = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &opt, sizeof(DWORD));

	connect = WinHttpConnect(session, host_w,
		(scheme == MTY_NET_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!connect) {
		MTY_Log("'WinHttpConnect' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_REQUEST;
		goto except;
	}

	//http request
	WCHAR path_w[512];
	_snwprintf_s(path_w, 512, _TRUNCATE, L"%hs", path);

	WCHAR method_w[32];
	_snwprintf_s(method_w, 32, _TRUNCATE, L"%hs", method);

	request = WinHttpOpenRequest(connect, method_w, path_w, NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, (scheme == MTY_NET_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
	if (!request) {
		MTY_Log("'WinHttpOpenRequest' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_REQUEST;
		goto except;
	}

	//headers & POST body
	WCHAR headers_w[512];

	if (headers)
		_snwprintf_s(headers_w, 512, _TRUNCATE, L"%hs", headers);

	success = WinHttpSendRequest(request,
		headers ? headers_w : WINHTTP_NO_ADDITIONAL_HEADERS,
		headers ? -1L : 0,
		(void *) body, body_len, body_len, 0);
	if (!success) {
		MTY_Log("'WinHttpSendRequest' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_REQUEST;
		goto except;
	}

	//response headers
	success = WinHttpReceiveResponse(request, NULL);
	if (!success) {
		MTY_Log("'WinHttpReceiveResponse' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_RESPONSE;
		goto except;
	}

	//query response headers
	WCHAR header_wstr[128];
	DWORD buf_len = 128 * sizeof(WCHAR);
	success = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, NULL,
		header_wstr, &buf_len, NULL);
	if (!success) {
		MTY_Log("'WinHttpQueryHeaders' failed with error 0x%X", GetLastError());
		r = MTY_NET_HTTP_ERR_RESPONSE;
		goto except;
	}

	status_code = _wtoi(header_wstr);

	//content encoding
	buf_len = 128 * sizeof(WCHAR);
	success = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_ENCODING, NULL,
		header_wstr, &buf_len, NULL);

	gzipped = success && !wcscmp(header_wstr, L"gzip");

	//response body
	r = mty_http_recv_response(request, response, response_len);
	if (r != MTY_NET_OK) goto except;

	//response decompression & null character
	if (*response && *response_len > 0) {
		(*response)[*response_len] = '\0';

		if (gzipped)
			r = mty_http_decompress(response, response_len);
	}

	except:

	if (request)
		WinHttpCloseHandle(request);

	if (connect)
		WinHttpCloseHandle(connect);

	if (session)
		WinHttpCloseHandle(session);

	if (r != MTY_NET_OK) {
		free(*response);
		*response_len = 0;
		*response = NULL;
	}

	return (r == MTY_NET_OK) ? status_code : r;
}
