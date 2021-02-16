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

struct mty_net {
	char *hout;
	struct http_header *hin;

	TCP_SOCKET socket;
	struct tls *tls;

	char *host;
	uint16_t port;
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

bool mty_net_write(struct mty_net *ucc, const void *buf, size_t size)
{
	return ucc->tls ? tls_write(ucc->tls, buf, size) : tcp_write(ucc->socket, buf, size);
}

bool mty_net_read(struct mty_net *ucc, void *buf, size_t size, uint32_t timeout)
{
	return ucc->tls ? tls_read(ucc->tls, buf, size, timeout) : tcp_read(ucc->socket, buf, size, timeout);
}

static int32_t mty_net_read_header_(struct mty_net *ucc, char **header, int32_t timeout)
{
	int32_t r = MTY_NET_ERR_MAX_HEADER;

	uint32_t max_header = LEN_HEADER;
	char *h = *header = calloc(max_header, 1);

	for (uint32_t x = 0; x < max_header - 1; x++) {
		if (!mty_net_read(ucc, h + x, 1, timeout))
			{r = MTY_NET_ERR_DEFAULT; break;}

		if (x > 2 && h[x - 3] == '\r' && h[x - 2] == '\n' && h[x - 1] == '\r' && h[x] == '\n')
			return MTY_NET_OK;
	}

	free(h);
	*header = NULL;

	return r;
}

int32_t mty_net_read_header(struct mty_net *ucc, int32_t timeout)
{
	//free any exiting response headers
	http_header_destroy(&ucc->hin);

	//read the HTTP response header
	char *header = NULL;
	int32_t e = mty_net_read_header_(ucc, &header, timeout);

	if (e == MTY_NET_OK) {
		//parse the header into the http_header struct
		ucc->hin = http_parse_header(header);
		free(header);
	}

	return e;
}

bool mty_net_get_status_code(struct mty_net *ucc, uint16_t *status_code)
{
	*status_code = 0;
	return http_get_status_code(ucc->hin, status_code);
}

struct mty_net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout)
{
	bool r = true;

	struct mty_net *ctx = MTY_Alloc(1, sizeof(struct mty_net));
	ctx->host = MTY_Strdup(host);
	ctx->port = port;

	MTY_GlobalLock(&NET_GLOCK);

	bool psecure = false;
	uint16_t pport = 0;
	char *phost = NULL;
	char *ppath = NULL;

	bool ok = http_parse_url(NET_PROXY, &psecure, &phost, &pport, &ppath);

	MTY_GlobalUnlock(&NET_GLOCK);

	bool use_proxy = ok && phost && phost[0];

	//connect via proxy if specified
	const char *use_host = use_proxy ? phost : host;
	uint16_t use_port = use_proxy ? pport : port;

	//resolve the hostname into an ip4 address
	char ip4[LEN_IP4];
	r = dns_query(use_host, ip4, LEN_IP4);

	if (use_proxy) {
		MTY_Free(phost);
		MTY_Free(ppath);
	}

	if (!r)
		goto except;

	//make the socket connection
	ctx->socket = tcp_connect(ip4, use_port, timeout);
	if (ctx->socket == TCP_INVALID_SOCKET) {
		r = false;
		goto except;
	}

	//proxy CONNECT request
	if (use_proxy) {
		char *h = http_connect(ctx->host, ctx->port, NULL);

		//write the header to the HTTP client/server
		r = mty_net_write(ctx, h, strlen(h));
		MTY_Free(h);

		if (!r)
			goto except;

		//read the response header
		int32_t e = mty_net_read_header(ctx, timeout);
		if (e != MTY_NET_OK) {
			r = false;
			goto except;
		}

		//get the status code
		uint16_t status_code = 0;
		r = mty_net_get_status_code(ctx, &status_code);
		if (!r)
			goto except;

		if (status_code != 200) {
			r = false;
			goto except;
		}
	}

	if (secure) {
		ctx->tls = tls_connect(ctx->socket, ctx->host, timeout);
		if (!ctx->tls) {
			r = false;
			goto except;
		}
	}

	except:

	if (!r)
		mty_net_destroy(&ctx);

	return ctx;
}

struct mty_net *mty_net_listen(const char *ip, uint16_t port)
{
	struct mty_net *ctx = MTY_Alloc(1, sizeof(struct mty_net));
	ctx->port = port;

	ctx->socket = tcp_listen(ip, ctx->port);
	if (ctx->socket == TCP_INVALID_SOCKET)
		mty_net_destroy(&ctx);

	return ctx;
}

struct mty_net *mty_net_accept(struct mty_net *ucc, bool secure, uint32_t timeout)
{
	bool r = true;
	struct mty_net *child = MTY_Alloc(1, sizeof(struct mty_net));

	child->socket = tcp_accept(ucc->socket, timeout);
	if (child->socket == TCP_INVALID_SOCKET) {
		r = false;
		goto except;
	}

	if (secure) {
		child->tls = tls_accept(child->socket, timeout);
		if (!child->tls) {
			r = false;
			goto except;
		}
	}

	except:

	if (!r)
		mty_net_destroy(&child);

	return child;
}

MTY_Async mty_net_poll(struct mty_net *ucc, uint32_t timeout)
{
	return tcp_poll(ucc->socket, false, timeout);
}

void mty_net_destroy(struct mty_net **net)
{
	if (!net || !*net)
		return;

	struct mty_net *ctx = *net;

	tls_destroy(&ctx->tls);
	tcp_destroy(&ctx->socket);

	http_header_destroy(&ctx->hin);

	MTY_Free(ctx->host);
	MTY_Free(ctx->hout);

	free(ctx);
	*net = NULL;
}


// Request

void mty_net_set_header_str(struct mty_net *ucc, const char *name, const char *value)
{
	ucc->hout = http_set_header_str(ucc->hout, name, value);
}

void mty_net_set_header_int(struct mty_net *ucc, const char *name, int32_t value)
{
	ucc->hout = http_set_header_int(ucc->hout, name, value);
}

int32_t mty_net_write_header(struct mty_net *ucc, const char *str0, const char *str1, int32_t type)
{
	//generate the HTTP request/response header
	char *h = (type == MTY_NET_REQUEST) ?
		http_request(str0, ucc->host, str1, ucc->hout) :
		http_response(str0, str1, ucc->hout);

	//write the header to the HTTP client/server
	bool ok = mty_net_write(ucc, h, (uint32_t) strlen(h));

	free(h);

	return ok ? MTY_NET_OK : MTY_NET_ERR_DEFAULT;
}


// Response

bool mty_net_get_header_str(struct mty_net *ucc, const char *key, const char **val)
{
	return http_get_header_str(ucc->hin, key, val);
}

bool mty_net_get_header_int(struct mty_net *ucc, const char *key, int32_t *val)
{
	return http_get_header_int(ucc->hin, key, val);
}


// Utility

void mty_http_set_headers(struct mty_net *ucc, const char *header_str_orig)
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
