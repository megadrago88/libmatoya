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
#include "sec.h"
#include "http.h"

struct mty_net {
	char *host;
	TCP_SOCKET socket;
	struct tls *tls;
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
	ctx->socket = tcp_connect(ip, cport, timeout);
	if (ctx->socket == TCP_INVALID_SOCKET) {
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
	ctx->host = MTY_Strdup(ip);

	ctx->socket = tcp_listen(ip, port);
	if (ctx->socket == TCP_INVALID_SOCKET)
		mty_net_destroy(&ctx);

	return ctx;
}

struct mty_net *mty_net_accept(struct mty_net *ctx, bool secure, uint32_t timeout)
{
	bool r = true;
	struct mty_net *child = MTY_Alloc(1, sizeof(struct mty_net));
	child->host = MTY_Strdup(ctx->host);

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

void mty_net_destroy(struct mty_net **net)
{
	if (!net || !*net)
		return;

	struct mty_net *ctx = *net;

	tls_destroy(&ctx->tls);
	tcp_destroy(&ctx->socket);

	MTY_Free(ctx->host);

	MTY_Free(ctx);
	*net = NULL;
}

MTY_Async mty_net_poll(struct mty_net *ctx, uint32_t timeout)
{
	return tcp_poll(ctx->socket, false, timeout);
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
