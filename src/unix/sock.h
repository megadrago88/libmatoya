// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <errno.h>
#include <netdb.h>

#define mty_sock_error()     errno
#define MTY_SOCK_WOULD_BLOCK EAGAIN
#define MTY_SOCK_IN_PROGRESS EINPROGRESS
#define MTY_SOCK_BAD_FD      EBADF

#define closesocket    close
#define INVALID_SOCKET -1

typedef int32_t SOCKET;

static int32_t sock_set_nonblocking(SOCKET s)
{
	return fcntl(s, F_SETFL, O_NONBLOCK);
}
