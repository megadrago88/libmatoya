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

struct mty_net {
	char *host;
	uint16_t port;

	TCP_SOCKET socket;
	struct tls *tls;
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

static bool mty_net_proxy_connect(struct mty_net *ctx, uint32_t timeout)
{
	struct http_header *hdr = NULL;
	char *h = http_connect(ctx->host, ctx->port, NULL);

	//write the header to the HTTP client/server
	bool r = mty_net_write(ctx, h, strlen(h));
	MTY_Free(h);

	if (!r)
		goto except;

	//read the response header
	hdr = http_read_header(ctx, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	//get the status code
	uint16_t status_code = 0;
	r = http_get_status_code(hdr, &status_code);
	if (!r)
		goto except;

	if (status_code != 200) {
		r = false;
		goto except;
	}

	except:

	if (hdr)
		http_header_destroy(&hdr);

	return r;
}

struct mty_net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout)
{
	bool r = true;

	struct mty_net *ctx = MTY_Alloc(1, sizeof(struct mty_net));
	ctx->host = MTY_Strdup(host);
	ctx->port = port;

	// Use proxy host if globally set
	MTY_GlobalLock(&NET_GLOCK);

	bool psecure = false;
	uint16_t pport = 0;
	char *phost = NULL;
	char *ppath = NULL;

	bool ok = http_parse_url(NET_PROXY, &psecure, &phost, &pport, &ppath);

	MTY_GlobalUnlock(&NET_GLOCK);

	bool use_proxy = ok && phost && phost[0];

	const char *use_host = use_proxy ? phost : host;
	uint16_t use_port = use_proxy ? pport : port;

	// DNS resolve hostname into an ip address string
	char ip[64];
	r = dns_query(use_host, ip, 64);

	if (use_proxy) {
		MTY_Free(phost);
		MTY_Free(ppath);
	}

	if (!r)
		goto except;

	// Make the tcp connection
	ctx->socket = tcp_connect(ip, use_port, timeout);
	if (ctx->socket == TCP_INVALID_SOCKET) {
		r = false;
		goto except;
	}

	// HTTP proxy CONNECT request
	if (use_proxy) {
		r = mty_net_proxy_connect(ctx, timeout);
		if (!r)
			goto except;
	}

	// TLS handshake
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

struct mty_net *mty_net_accept(struct mty_net *ctx, bool secure, uint32_t timeout)
{
	bool r = true;
	struct mty_net *child = MTY_Alloc(1, sizeof(struct mty_net));

	child->socket = tcp_accept(ctx->socket, timeout);
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

MTY_Async mty_net_poll(struct mty_net *ctx, uint32_t timeout)
{
	return tcp_poll(ctx->socket, false, timeout);
}

void mty_net_destroy(struct mty_net **net)
{
	if (!net || !*net)
		return;

	struct mty_net *ctx = *net;

	tls_destroy(&ctx->tls);
	tcp_destroy(&ctx->socket);

	MTY_Free(ctx->host);

	free(ctx);
	*net = NULL;
}

bool mty_net_write(struct mty_net *ctx, const void *buf, size_t size)
{
	return ctx->tls ? tls_write(ctx->tls, buf, size) : tcp_write(ctx->socket, buf, size);
}

bool mty_net_read(struct mty_net *ctx, void *buf, size_t size, uint32_t timeout)
{
	return ctx->tls ? tls_read(ctx->tls, buf, size, timeout) : tcp_read(ctx->socket, buf, size, timeout);
}

const char *mty_net_get_host(struct mty_net *ctx)
{
	return ctx->host;
}
