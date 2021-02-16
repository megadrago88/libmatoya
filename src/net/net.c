// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "tcp.h"
#include "tls.h"
#include "http.h"

#define LEN_IP4    16
#define LEN_ORIGIN 512
#define LEN_HEADER (2 * 1024 * 1024)

struct mty_net_conn {
	char *hout;
	struct http_header *hin;

	TCP_SOCKET socket;
	struct tls *tls;

	char *host;
	uint16_t port;
};

struct mty_net_info {
	bool secure;
	char *host;
	uint16_t port;
	char *path;
};


// Global proxy setting

static MTY_Atomic32 NET_GLOCK;
static char NET_PROXY[MTY_URL_MAX];

bool MTY_HttpSetProxy(const char *proxy)
{
	MTY_GlobalLock(&NET_GLOCK);

	bool was_set = NET_PROXY[0];

	if (proxy) {
		if (!was_set) {
			snprintf(NET_PROXY, MTY_URL_MAX, "%s", proxy);
			was_set = true;
		}

	} else {
		NET_PROXY[0] = '\0';
	}

	MTY_GlobalUnlock(&NET_GLOCK);

	return was_set;
}

char *mty_net_get_proxy(void)
{
	char *proxy = NULL;

	MTY_GlobalLock(&NET_GLOCK);

	if (NET_PROXY[0])
		proxy = MTY_Strdup(NET_PROXY);

	MTY_GlobalUnlock(&NET_GLOCK);

	return proxy;
}


// TLS Context

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	return tls_load_cacert(cacert, size);
}


// Connection

struct mty_net_conn *mty_net_new_conn(void)
{
	return calloc(1, sizeof(struct mty_net_conn));
}

int32_t mty_net_write(struct mty_net_conn *ucc, const char *body, uint32_t body_len)
{
	return ucc->tls ? tls_write(ucc->tls, body, body_len) : tcp_write(ucc->socket, body, body_len);
}

int32_t mty_net_read(struct mty_net_conn *ucc, char *body, uint32_t body_len, int32_t timeout)
{
	return ucc->tls ? tls_read(ucc->tls, body, body_len, timeout) : tcp_read(ucc->socket, body, body_len, timeout);
}

static int32_t mty_net_read_header_(struct mty_net_conn *ucc, char **header, int32_t timeout_ms)
{
	int32_t r = MTY_NET_ERR_MAX_HEADER;

	uint32_t max_header = LEN_HEADER;
	char *h = *header = calloc(max_header, 1);

	for (uint32_t x = 0; x < max_header - 1; x++) {
		int32_t e = mty_net_read(ucc, h + x, 1, timeout_ms);
		if (e != MTY_NET_OK) {r = e; break;}

		if (x > 2 && h[x - 3] == '\r' && h[x - 2] == '\n' && h[x - 1] == '\r' && h[x] == '\n')
			return MTY_NET_OK;
	}

	free(h);
	*header = NULL;

	return r;
}

int32_t mty_net_read_header(struct mty_net_conn *ucc, int32_t timeout_ms)
{
	//free any exiting response headers
	if (ucc->hin) http_free_header(ucc->hin);
	ucc->hin = NULL;

	//read the HTTP response header
	char *header = NULL;
	int32_t e = mty_net_read_header_(ucc, &header, timeout_ms);

	if (e == MTY_NET_OK) {
		//parse the header into the http_header struct
		ucc->hin = http_parse_header(header);
		free(header);
	}

	return e;
}

int32_t mty_net_get_status_code(struct mty_net_conn *ucc, uint16_t *status_code)
{
	*status_code = 0;
	return http_get_status_code(ucc->hin, status_code);
}

int32_t mty_net_connect(struct mty_net_conn *ucc, bool secure, const char *host, uint16_t port,
	bool verify_host, int32_t timeout_ms)
{
	//set state
	ucc->host = MTY_Strdup(host);
	ucc->port = port;

	struct mty_net_info pi = {0};

	MTY_GlobalLock(&NET_GLOCK);

	bool ok = http_parse_url(NET_PROXY, &pi.secure, &pi.host, &pi.port, &pi.path) == MTY_NET_OK;

	MTY_GlobalUnlock(&NET_GLOCK);

	bool use_proxy = ok && pi.host && pi.host[0];

	//connect via proxy if specified
	const char *use_host = use_proxy ? pi.host : host;
	uint16_t use_port = use_proxy ? pi.port : port;

	//resolve the hostname into an ip4 address
	char ip4[LEN_IP4];
	ok = dns_query(use_host, ip4, LEN_IP4);

	if (use_proxy) {
		free(pi.host);
		free(pi.path);
	}

	if (!ok)
		return MTY_NET_ERR_DEFAULT;

	//make the socket connection
	ucc->socket = tcp_connect(ip4, use_port, timeout_ms);
	if (ucc->socket == tcp_invalid_socket())
		return MTY_NET_ERR_DEFAULT;

	//proxy CONNECT request
	if (use_proxy) {
		char *h = http_connect(ucc->host, ucc->port, NULL);

		//write the header to the HTTP client/server
		int32_t e = mty_net_write(ucc, h, (uint32_t) strlen(h));
		free(h);
		if (e != MTY_NET_OK) return e;

		//read the response header
		e = mty_net_read_header(ucc, timeout_ms);
		if (e != MTY_NET_OK) return e;

		//get the status code
		uint16_t status_code = 0;
		e = mty_net_get_status_code(ucc, &status_code);
		if (e != MTY_NET_OK) return e;
		if (status_code != 200) return MTY_NET_ERR_PROXY;
	}

	if (secure) {
		ucc->tls = tls_connect(ucc->socket, ucc->host, verify_host, timeout_ms);
		if (!ucc->tls)
			return MTY_NET_ERR_DEFAULT;
	}

	return MTY_NET_OK;
}

int32_t mty_net_listen(struct mty_net_conn *ucc, const char *bind_ip4, uint16_t port)
{
	ucc->port = port;
	ucc->socket = tcp_listen(bind_ip4, ucc->port);
	if (ucc->socket == tcp_invalid_socket())
		return MTY_NET_ERR_DEFAULT;

	return MTY_NET_OK;
}

int32_t mty_net_accept(struct mty_net_conn *ucc, struct mty_net_conn **ucc_new_in, bool secure, int32_t timeout_ms)
{
	struct mty_net_conn *ucc_new = *ucc_new_in = mty_net_new_conn();

	int32_t r = MTY_NET_OK;

	ucc_new->socket = tcp_accept(ucc->socket, timeout_ms);
	if (ucc_new->socket == tcp_invalid_socket()) {r = MTY_NET_ERR_DEFAULT; goto except;}

	if (secure) {
		ucc_new->tls = tls_accept(ucc_new->socket, timeout_ms);
		if (!ucc_new->tls) {r = MTY_NET_ERR_DEFAULT; goto except;}
	}

	except:

	if (r != MTY_NET_OK) {
		free(ucc_new);
		*ucc_new_in = NULL;
	}

	return r;
}

int32_t mty_net_poll(struct mty_net_conn *ucc, int32_t timeout_ms)
{
	return tcp_poll(ucc->socket, TCP_POLLIN, timeout_ms);
}

void mty_net_close(struct mty_net_conn *ucc)
{
	if (!ucc) return;

	tls_destroy(&ucc->tls);
	tcp_destroy(&ucc->socket);

	free(ucc->host);
	http_free_header(ucc->hin);
	free(ucc->hout);
	free(ucc);
}


// Request

void mty_net_set_header_str(struct mty_net_conn *ucc, const char *name, const char *value)
{
	ucc->hout = http_set_header(ucc->hout, name, HTTP_STRING, value);
}

void mty_net_set_header_int(struct mty_net_conn *ucc, const char *name, int32_t value)
{
	ucc->hout = http_set_header(ucc->hout, name, HTTP_INT, &value);
}

int32_t mty_net_write_header(struct mty_net_conn *ucc, const char *str0, const char *str1, int32_t type)
{
	//generate the HTTP request/response header
	char *h = (type == MTY_NET_REQUEST) ?
		http_request(str0, ucc->host, str1, ucc->hout) :
		http_response(str0, str1, ucc->hout);

	//write the header to the HTTP client/server
	int32_t e = mty_net_write(ucc, h, (uint32_t) strlen(h));

	free(h);

	return e;
}


// Response

int32_t mty_net_get_header(struct mty_net_conn *ucc, const char *key, int32_t *val_int, char **val_str)
{
	return http_get_header(ucc->hin, key, val_int, val_str);
}


// Utility

bool MTY_HttpParseUrl(const char *url, char *host, size_t hostSize, char *path, size_t pathSize)
{
	bool r = false;
	struct mty_net_info uci = {0};

	if (http_parse_url(url, &uci.secure, &uci.host, &uci.port, &uci.path) == MTY_NET_OK) {
		snprintf(host, hostSize, "%s", uci.host);
		snprintf(path, pathSize, "%s", uci.path);
		r = true;
	}

	free(uci.host);
	free(uci.path);

	return r;
}

void MTY_HttpEncodeUrl(const char *src, char *dst, size_t size)
{
	char table[256];
	dst[0] = '\0';

	for (int32_t i = 0; i < 256; i++)
		table[i] = (char) (isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ?
			i : (i == ' ') ? '+' : 0);

	for (size_t x = 0; x < strlen(src); x++){
		int32_t c = src[x];
		int32_t inc = 1;

		if (table[c]) {
			snprintf(dst, size, "%c", table[c]);
		} else {
			snprintf(dst, size, "%%%02X", c);
			inc = 3;
		}

		dst += inc;
		size -= inc;
	}
}

void mty_http_set_headers(struct mty_net_conn *ucc, const char *header_str_orig)
{
	char *tok, *ptr = NULL;
	char *header_str = MTY_Strdup(header_str_orig);

	tok = MTY_Strtok(header_str, "\n", &ptr);
	while (tok) {
		char *key, *val = NULL, *ptr2 = NULL;

		key = MTY_Strtok(tok, " :", &ptr2);
		if (key)
			val = MTY_Strtok(NULL, "", &ptr2);

		if (key && val) {
			mty_net_set_header_str(ucc, key, val);
		} else break;

		tok = MTY_Strtok(NULL, "\n", &ptr);
	}

	free(header_str);
}
