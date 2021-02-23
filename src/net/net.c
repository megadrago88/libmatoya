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

struct mty_net {
	char *host;
	intptr_t socket;
	struct secure *sec;
};

struct mty_net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout)
{
	struct mty_net *ctx = MTY_Alloc(1, sizeof(struct mty_net));
	ctx->host = MTY_Strdup(host);

	// Check global HTTP proxy settings
	const char *chost = ctx->host;
	uint16_t cport = port;
	bool use_proxy = http_should_proxy(&chost, &cport);

	// DNS resolve hostname into an ip address string
	char ip[64];
	bool r = dns_query(chost, ip, 64);
	if (!r)
		goto except;

	// Make the tcp connection
	ctx->socket = mty_tcp_connect(ip, cport, timeout);
	if (ctx->socket == -1) {
		r = false;
		goto except;
	}

	// HTTP proxy CONNECT request
	if (use_proxy) {
		r = http_proxy_connect(ctx, port, timeout);
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

struct mty_net *mty_net_listen(const char *ip, uint16_t port)
{
	struct mty_net *ctx = MTY_Alloc(1, sizeof(struct mty_net));
	ctx->host = MTY_Strdup(ip);

	ctx->socket = mty_tcp_listen(ip, port);
	if (ctx->socket == -1)
		mty_net_destroy(&ctx);

	return ctx;
}

struct mty_net *mty_net_accept(struct mty_net *ctx, uint32_t timeout)
{
	intptr_t s = mty_tcp_accept(ctx->socket, timeout);

	if (s != -1) {
		struct mty_net *child = MTY_Alloc(1, sizeof(struct mty_net));
		child->host = MTY_Strdup(ctx->host);
		child->socket = s;

		return child;
	}

	return NULL;
}

void mty_net_destroy(struct mty_net **net)
{
	if (!net || !*net)
		return;

	struct mty_net *ctx = *net;

	mty_secure_destroy(&ctx->sec);
	mty_tcp_destroy(&ctx->socket);

	MTY_Free(ctx->host);

	MTY_Free(ctx);
	*net = NULL;
}

MTY_Async mty_net_poll(struct mty_net *ctx, uint32_t timeout)
{
	return mty_tcp_poll(ctx->socket, false, timeout);
}

bool mty_net_write(struct mty_net *ctx, const void *buf, size_t size)
{
	return ctx->sec ? mty_secure_write(ctx->sec, buf, size) : mty_tcp_write(ctx->socket, buf, size);
}

bool mty_net_read(struct mty_net *ctx, void *buf, size_t size, uint32_t timeout)
{
	return ctx->sec ? mty_secure_read(ctx->sec, buf, size, timeout) : mty_tcp_read(ctx->socket, buf, size, timeout);
}

const char *mty_net_get_host(struct mty_net *ctx)
{
	return ctx->host;
}
