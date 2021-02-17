// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "tcp.h"

#include <stdlib.h>
#include <string.h>

#include "net/sock.h"


// Connect, accept

static bool tcp_socket_ok(TCP_SOCKET s)
{
	int32_t opt = 0;
	socklen_t size = sizeof(int32_t);
	int32_t e = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &opt, &size);

	return e == 0 && opt == 0;
}

static void tcp_set_sockopt(TCP_SOCKET s, int32_t level, int32_t opt_name, int32_t val)
{
	setsockopt(s, level, opt_name, (const char *) &val, sizeof(int32_t));
}

static void tcp_set_options(TCP_SOCKET s)
{
	tcp_set_sockopt(s, SOL_SOCKET, SO_RCVBUF, 64 * 1024);
	tcp_set_sockopt(s, SOL_SOCKET, SO_SNDBUF, 64 * 1024);
	tcp_set_sockopt(s, SOL_SOCKET, SO_KEEPALIVE, 1);
	tcp_set_sockopt(s, SOL_SOCKET, SO_REUSEADDR, 1);

	tcp_set_sockopt(s, IPPROTO_TCP, TCP_NODELAY, 1);
}

static TCP_SOCKET tcp_socket(const char *ip, uint16_t port, struct sockaddr_in *addr)
{
	TCP_SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return s;

	if (!sock_set_nonblocking(s)) {
		tcp_destroy(&s);
		return s;
	}

	tcp_set_options(s);

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);

	if (ip) {
		inet_pton(AF_INET, ip, &addr->sin_addr);

	} else {
		addr->sin_addr.s_addr = INADDR_ANY;
	}

	return s;
}

TCP_SOCKET tcp_connect(const char *ip, uint16_t port, uint32_t timeout)
{
	struct sockaddr_in addr = {0};

	TCP_SOCKET s = tcp_socket(ip, port, &addr);
	if (s == INVALID_SOCKET)
		return s;

	bool r = true;

	// TODO Check return value?
	connect(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

	// Initial socket state must be 'in progress' for nonblocking connect
	if (mty_sock_error() != MTY_SOCK_IN_PROGRESS) {
		r = false;
		goto except;
	}

	// Wait for socket to be ready to write
	if (tcp_poll(s, true, timeout) != MTY_ASYNC_OK) {
		r = false;
		goto except;
	}

	// If the socket is clear of errors, we made a successful connection
	r = tcp_socket_ok(s);
	if (!r)
		goto except;

	except:

	if (!r)
		tcp_destroy(&s);

	return s;
}

TCP_SOCKET tcp_listen(const char *ip, uint16_t port)
{
	struct sockaddr_in addr = {0};

	TCP_SOCKET s = tcp_socket(ip, port, &addr);
	if (s == INVALID_SOCKET)
		return s;

	bool r = true;

	if (bind(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != 0) {
		MTY_Log("'bind' failed with errno %d", mty_sock_error());
		r = false;
		goto except;
	}

	if (listen(s, SOMAXCONN) != 0) {
		MTY_Log("'listen' failed with errno %d", mty_sock_error());
		r = false;
		goto except;
	}

	except:

	if (!r)
		tcp_destroy(&s);

	return s;
}

TCP_SOCKET tcp_accept(TCP_SOCKET s, uint32_t timeout)
{
	if (tcp_poll(s, false, timeout) == MTY_ASYNC_OK) {
		TCP_SOCKET child = accept(s, NULL, NULL);

		if (child == INVALID_SOCKET) {
			MTY_Log("'accept' failed with errno %d", mty_sock_error());
			return child;
		}

		if (!sock_set_nonblocking(child)) {
			tcp_destroy(&child);
			return child;
		}

		tcp_set_options(child);
	}

	return INVALID_SOCKET;
}

void tcp_destroy(TCP_SOCKET *socket)
{
	if (!socket)
		return;

	TCP_SOCKET s = *socket;

	if (s != INVALID_SOCKET) {
		shutdown(s, SHUT_RDWR);
		closesocket(s);

		*socket = INVALID_SOCKET;
	}
}


// Poll, read, write

MTY_Async tcp_async(void)
{
	int32_t e = mty_sock_error();

	return e == MTY_SOCK_WOULD_BLOCK ? MTY_ASYNC_CONTINUE :
		e == MTY_SOCK_BAD_FD ? MTY_ASYNC_ERROR : MTY_ASYNC_OK;
}

MTY_Async tcp_poll(TCP_SOCKET s, bool out, uint32_t timeout)
{
	struct pollfd fd = {0};
	fd.events = out ? POLLOUT : POLLIN;
	fd.fd = s;

	int32_t e = poll(&fd, 1, timeout);

	return e == 0 ? MTY_ASYNC_CONTINUE : e < 0 ? MTY_ASYNC_ERROR : MTY_ASYNC_OK;
}

bool tcp_write(TCP_SOCKET s, const void *buf, size_t size)
{
	for (size_t total = 0; total < size;) {
		int32_t n = send(s, (const char *) buf + total, (int32_t) (size - total), 0);

		if (n <= 0)
			return false;

		total += n;
	}

	return true;
}

bool tcp_read(TCP_SOCKET s, void *buf, size_t size, uint32_t timeout)
{
	for (size_t total = 0; total < size;) {
		if (tcp_poll(s, false, timeout) != MTY_ASYNC_OK)
			return false;

		int32_t n = recv(s, (char *) buf + total, (int32_t) (size - total), 0);

		if (n <= 0) {
			if (mty_sock_error() != MTY_SOCK_WOULD_BLOCK)
				return false;

		} else {
			total += n;
		}
	}

	return true;
}


// DNS

bool dns_query(const char *host, char *ip, size_t size)
{
	bool r = true;

	// IP4 only
	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *servinfo = NULL;
	int32_t e = getaddrinfo(host, NULL, &hints, &servinfo);
	if (e != 0) {
		r = false;
		goto except;
	}

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
