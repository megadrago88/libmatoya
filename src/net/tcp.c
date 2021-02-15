// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <string.h>

#include "tcp.h"
#include "net/sock.h"

struct tcp_context {
	SOCKET s;
};

static int32_t tcp_get_error(SOCKET s)
{
	int32_t opt = 0;
	socklen_t size = sizeof(int32_t);
	int32_t e = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &opt, &size);

	return e ? e : opt;
}

int32_t tcp_error(void)
{
	return mty_sock_error();
}

int32_t tcp_would_block(void)
{
	return MTY_SOCK_WOULD_BLOCK;
}

int32_t tcp_in_progress(void)
{
	return MTY_SOCK_IN_PROGRESS;
}

int32_t tcp_bad_fd(void)
{
	return MTY_SOCK_BAD_FD;
}

void tcp_close(struct tcp_context *nc)
{
	if (!nc) return;

	if (nc->s != INVALID_SOCKET) {
		shutdown(nc->s, SHUT_RDWR);
		closesocket(nc->s);
	}

	free(nc);
}

static void tcp_set_sockopt(SOCKET s, int32_t level, int32_t opt_name, int32_t val)
{
	setsockopt(s, level, opt_name, (const char *) &val, sizeof(int32_t));
}

static void tcp_set_options(SOCKET s)
{
	tcp_set_sockopt(s, SOL_SOCKET, SO_RCVBUF, 64 * 1024);
	tcp_set_sockopt(s, SOL_SOCKET, SO_SNDBUF, 64 * 1024);
	tcp_set_sockopt(s, SOL_SOCKET, SO_KEEPALIVE, 1);
	tcp_set_sockopt(s, IPPROTO_TCP, TCP_NODELAY, 1);
	tcp_set_sockopt(s, SOL_SOCKET, SO_REUSEADDR, 1);
}

int32_t tcp_poll(struct tcp_context *nc, int32_t tcp_event, int32_t timeout_ms)
{
	struct pollfd fd;
	memset(&fd, 0, sizeof(struct pollfd));

	fd.fd = nc->s;
	fd.events = (tcp_event == TCP_POLLIN) ? POLLIN : (tcp_event == TCP_POLLOUT) ? POLLOUT : 0;

	int32_t e = poll(&fd, 1, timeout_ms);

	return (e == 0) ? MTY_NET_TCP_ERR_TIMEOUT : (e < 0) ? MTY_NET_TCP_ERR_POLL : MTY_NET_TCP_OK;
}

static int32_t tcp_setup(struct tcp_context *nc, const char *ip4, uint16_t port, struct sockaddr_in *addr)
{
	nc->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (nc->s == INVALID_SOCKET) return MTY_NET_TCP_ERR_SOCKET;

	//put socket in nonblocking mode, allows us to implement connection timeout
	int32_t e = sock_set_nonblocking(nc->s);
	if (e != 0) return MTY_NET_TCP_ERR_BLOCKMODE;

	tcp_set_options(nc->s);

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);

	if (ip4) {
		inet_pton(AF_INET, ip4, &addr->sin_addr);
	} else {
		addr->sin_addr.s_addr = INADDR_ANY;
	}

	return MTY_NET_TCP_OK;
}

int32_t tcp_connect(struct tcp_context **nc_out, const char *ip4, uint16_t port, int32_t timeout_ms)
{
	int32_t r = MTY_NET_TCP_OK;

	struct tcp_context *nc = *nc_out = calloc(1, sizeof(struct tcp_context));

	struct sockaddr_in addr;
	int32_t e = tcp_setup(nc, ip4, port, &addr);
	if (e != MTY_NET_TCP_OK) {r = e; goto except;}

	//initiate the socket connection
	e = connect(nc->s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

	//initial socket state must be 'in progress' for nonblocking connect
	if (tcp_error() != tcp_in_progress()) {r = MTY_NET_TCP_ERR_CONNECT; goto except;}

	//wait for socket to be ready to write
	e = tcp_poll(nc, TCP_POLLOUT, timeout_ms);
	if (e != MTY_NET_TCP_OK) {r = e; goto except;}

	//if the socket is clear of errors, we made a successful connection
	if (tcp_get_error(nc->s) != 0) {r = MTY_NET_TCP_ERR_CONNECT_FINAL; goto except;}

	except:

	if (r != MTY_NET_TCP_OK) {
		tcp_close(nc);
		*nc_out = NULL;
	}

	return r;
}

int32_t tcp_listen(struct tcp_context **nc_out, const char *bind_ip4, uint16_t port)
{
	int32_t r = MTY_NET_TCP_OK;

	struct tcp_context *nc = *nc_out = calloc(1, sizeof(struct tcp_context));

	struct sockaddr_in addr;
	int32_t e = tcp_setup(nc, bind_ip4, port, &addr);
	if (e != MTY_NET_TCP_OK) {r = e; goto except;}

	e = bind(nc->s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
	if (e != 0) {r = MTY_NET_TCP_ERR_BIND; goto except;}

	e = listen(nc->s, SOMAXCONN);
	if (e != 0) {r = MTY_NET_TCP_ERR_LISTEN; goto except;}

	except:

	if (r != MTY_NET_TCP_OK) {
		tcp_close(nc);
		*nc_out = NULL;
	}

	return r;
}

int32_t tcp_accept(struct tcp_context *nc, struct tcp_context **child, int32_t timeout_ms)
{
	int32_t e = tcp_poll(nc, TCP_POLLIN, timeout_ms);

	if (e == MTY_NET_TCP_OK) {
		SOCKET s = accept(nc->s, NULL, NULL);
		if (s == INVALID_SOCKET) return MTY_NET_TCP_ERR_ACCEPT;

		e = sock_set_nonblocking(s);
		if (e != 0) return MTY_NET_TCP_ERR_BLOCKMODE;

		tcp_set_options(s);

		struct tcp_context *new = *child = calloc(1, sizeof(struct tcp_context));
		new->s = s;

		return MTY_NET_TCP_OK;
	}

	return e;
}

int32_t tcp_write(void *ctx, const char *buf, size_t size)
{
	struct tcp_context *nc = (struct tcp_context *) ctx;

	for (size_t total = 0; total < size;) {
		int32_t n = send(nc->s, buf + total, (int32_t) (size - total), 0);
		if (n <= 0) return MTY_NET_TCP_ERR_WRITE;

		total += n;
	}

	return MTY_NET_TCP_OK;
}

int32_t tcp_read(void *ctx, char *buf, size_t size, int32_t timeout_ms)
{
	struct tcp_context *nc = (struct tcp_context *) ctx;

	for (size_t total = 0; total < size;) {
		int32_t e = tcp_poll(nc, TCP_POLLIN, timeout_ms);
		if (e != MTY_NET_TCP_OK) return e;

		int32_t n = recv(nc->s, buf + total, (int32_t) (size - total), 0);
		if (n <= 0) {
			if (tcp_error() == tcp_would_block()) continue;

			return (n == 0) ? MTY_NET_TCP_ERR_CLOSED : MTY_NET_TCP_ERR_READ;
		}

		total += n;
	}

	return MTY_NET_TCP_OK;
}

void tcp_get_socket(struct tcp_context *nc, void *socket)
{
	SOCKET *s = (SOCKET *) socket;

	*s = nc->s;
}


// DNS

bool dns_query(const char *host, char *ip, size_t size)
{
	bool r = true;

	// Set to request only IP4, TCP
	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *servinfo = NULL;
	int32_t e = getaddrinfo(host, NULL, &hints, &servinfo);
	if (e != 0) {
		MTY_Log("'getaddrinfo' failed with error %d", e);
		r = false;
		goto except;
	}

	// Attempt to convert the first returned address into string
	struct sockaddr_in *addr = (struct sockaddr_in *) servinfo->ai_addr;
	if (!inet_ntop(AF_INET, &addr->sin_addr, ip, size)) {
		MTY_Log("'inet_ntop' failed with errno %d", errno);
		r = false;
		goto except;
	}

	except:

	if (servinfo)
		freeaddrinfo(servinfo);

	return r;
}
