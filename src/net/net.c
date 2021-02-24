// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "net.h"

#include <stdio.h>
#include <string.h>

#include "tcp.h"
#include "http.h"
#include "secure.h"

struct net {
	char *host;
	TCP_SOCKET socket;
	struct secure *sec;
};

struct net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout)
{
	struct net *ctx = MTY_Alloc(1, sizeof(struct net));
	ctx->host = MTY_Strdup(host);

	// Check global HTTP proxy settings
	const char *chost = ctx->host;
	uint16_t cport = port;
	bool use_proxy = mty_http_should_proxy(&chost, &cport);

	// DNS resolve hostname into an ip address string
	char ip[64];
	bool r = mty_dns_query(chost, ip, 64);
	if (!r)
		goto except;

	// Make the tcp connection
	ctx->socket = mty_tcp_connect(ip, cport, timeout);
	if (ctx->socket == TCP_INVALID_SOCKET) {
		r = false;
		goto except;
	}

	// HTTP proxy CONNECT request
	if (use_proxy) {
		r = mty_http_proxy_connect(ctx, port, timeout);
		if (!r)
			goto except;
	}

	// TLS handshake
	if (secure) {
		ctx->sec = mty_secure_connect(ctx->socket, ctx->host, timeout);
		if (!ctx->sec) {
			r = false;
			goto except;
		}
	}

	except:

	if (!r)
		mty_net_destroy(&ctx);

	return ctx;
}

struct net *mty_net_listen(const char *ip, uint16_t port)
{
	TCP_SOCKET s = mty_tcp_listen(ip, port);

	if (s != TCP_INVALID_SOCKET) {
		struct net *ctx = MTY_Alloc(1, sizeof(struct net));
		ctx->host = MTY_Strdup(ip);
		ctx->socket = s;

		return ctx;
	}

	return NULL;
}

struct net *mty_net_accept(struct net *ctx, uint32_t timeout)
{
	TCP_SOCKET s = mty_tcp_accept(ctx->socket, timeout);

	if (s != TCP_INVALID_SOCKET) {
		struct net *child = MTY_Alloc(1, sizeof(struct net));
		child->host = MTY_Strdup(ctx->host);
		child->socket = s;

		return child;
	}

	return NULL;
}

void mty_net_destroy(struct net **net)
{
	if (!net || !*net)
		return;

	struct net *ctx = *net;

	mty_secure_destroy(&ctx->sec);
	mty_tcp_destroy(&ctx->socket);

	MTY_Free(ctx->host);

	MTY_Free(ctx);
	*net = NULL;
}

MTY_Async mty_net_poll(struct net *ctx, uint32_t timeout)
{
	return mty_tcp_poll(ctx->socket, false, timeout);
}

bool mty_net_write(struct net *ctx, const void *buf, size_t size)
{
	return ctx->sec ? mty_secure_write(ctx->sec, ctx->socket, buf, size) :
		mty_tcp_write(ctx->socket, buf, size);
}

bool mty_net_read(struct net *ctx, void *buf, size_t size, uint32_t timeout)
{
	return ctx->sec ? mty_secure_read(ctx->sec, ctx->socket, buf, size, timeout) :
		mty_tcp_read(ctx->socket, buf, size, timeout);
}

const char *mty_net_get_host(struct net *ctx)
{
	return ctx->host;
}
