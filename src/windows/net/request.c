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

static bool mty_http_recv_response(HINTERNET request, void **response, size_t *responseSize)
{
	DWORD available = 0;

	do {
		BOOL success = WinHttpQueryDataAvailable(request, &available);
		if (!success) {
			MTY_Log("'WinHttpQueryDataAvailable' failed with error 0x%X", GetLastError());
			return false;
		}

		char *buf = malloc(available);

		DWORD read = 0;
		success = WinHttpReadData(request, buf, available, &read);

		if (success && read > 0) {
			*response = realloc(*response, *responseSize + read + 1);
			memcpy((uint8_t *) *response + *responseSize, buf, read);
			*responseSize += read;
		}

		free(buf);

		if (!success) {
			MTY_Log("'WinHttpReadData' failed with error 0x%X", GetLastError());
			return false;
		}

	} while (available > 0);

	return true;
}

static bool mty_http_decompress(void **response, size_t *responseSize)
{
	size_t response_len_z = 0;

	char *response_z = mty_gzip_decompress(*response, *responseSize, &response_len_z);

	if (response_z) {
		free(*response);

		*response = response_z;
		*responseSize = response_len_z;

		return true;
	}

	return false;
}

bool MTY_HttpRequest(const char *method, const char *headers, MTY_Scheme scheme,
	const char *host, const char *path, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status)
{
	HINTERNET session = NULL, connect = NULL, request = NULL;

	*responseSize = 0;
	*response = NULL;

	bool ok = true;
	bool gzipped = false;

	//context initialization
	session = WinHttpOpen(L"mty-winhttp/v4", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) {
		MTY_Log("'WinHttpOpen' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//set timeouts
	BOOL success = WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);
	if (!success) {
		MTY_Log("'WinHttpSetTimeouts' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//tcp connection
	WCHAR host_w[512];
	_snwprintf_s(host_w, 512, _TRUNCATE, L"%hs", host);

	//attempt to force TLS 1.2, ignore failure
	DWORD opt = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &opt, sizeof(DWORD));

	connect = WinHttpConnect(session, host_w,
		(scheme == MTY_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!connect) {
		MTY_Log("'WinHttpConnect' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//http request
	WCHAR path_w[512];
	_snwprintf_s(path_w, 512, _TRUNCATE, L"%hs", path);

	WCHAR method_w[32];
	_snwprintf_s(method_w, 32, _TRUNCATE, L"%hs", method);

	request = WinHttpOpenRequest(connect, method_w, path_w, NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, (scheme == MTY_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
	if (!request) {
		MTY_Log("'WinHttpOpenRequest' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//headers & POST body
	WCHAR headers_w[512];

	if (headers)
		_snwprintf_s(headers_w, 512, _TRUNCATE, L"%hs", headers);

	success = WinHttpSendRequest(request,
		headers ? headers_w : WINHTTP_NO_ADDITIONAL_HEADERS,
		headers ? -1L : 0,
		(void *) body, (DWORD) bodySize, (DWORD) bodySize, 0);
	if (!success) {
		MTY_Log("'WinHttpSendRequest' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//response headers
	success = WinHttpReceiveResponse(request, NULL);
	if (!success) {
		MTY_Log("'WinHttpReceiveResponse' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	//query response headers
	WCHAR header_wstr[128];
	DWORD buf_len = 128 * sizeof(WCHAR);
	success = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, NULL,
		header_wstr, &buf_len, NULL);
	if (!success) {
		MTY_Log("'WinHttpQueryHeaders' failed with error 0x%X", GetLastError());
		ok = false;
		goto except;
	}

	*status = (uint16_t) _wtoi(header_wstr);

	//content encoding
	buf_len = 128 * sizeof(WCHAR);
	success = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_ENCODING, NULL,
		header_wstr, &buf_len, NULL);

	gzipped = success && !wcscmp(header_wstr, L"gzip");

	//response body
	ok = mty_http_recv_response(request, response, responseSize);
	if (!ok) goto except;

	//response decompression & null character
	if (*response && *responseSize > 0) {
		((uint8_t *) *response)[*responseSize] = '\0';

		if (gzipped)
			ok = mty_http_decompress(response, responseSize);
	}

	except:

	if (request)
		WinHttpCloseHandle(request);

	if (connect)
		WinHttpCloseHandle(connect);

	if (session)
		WinHttpCloseHandle(session);

	if (!ok) {
		free(*response);
		*responseSize = 0;
		*response = NULL;
	}

	return ok;
}
