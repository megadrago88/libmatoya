// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "matoya.h"

struct thread_args {
	// Request
	char method[32];
	bool secure;
	char host[128];
	char path[1024];
	char headers[1024];
	char body[5 * 1024];
	size_t body_len;
	uint32_t timeout_ms;

	// Response
	MTY_Async status;
	uint16_t code;
	void *res_body;
	size_t  res_body_len;
	MTY_HttpAsyncFunc cb;
};

static struct mty_http_async {
	MTY_ThreadPool *pool;
} *CTX;

static void mty_http_async_free_ta(void *opaque)
{
	struct thread_args *ta = (struct thread_args *) opaque;

	if (ta) {
		free(ta->res_body);
		free(ta);
	}
}

void MTY_HttpAsyncCreate(uint32_t maxThreads)
{
	if (CTX)
		return;

	CTX = calloc(1, sizeof(struct mty_http_async));

	CTX->pool = MTY_ThreadPoolCreate(maxThreads);
}

void MTY_HttpAsyncDestroy(void)
{
	if (!CTX)
		return;

	MTY_ThreadPoolDestroy(&CTX->pool, mty_http_async_free_ta);

	free(CTX);
	CTX = NULL;
}

static void mty_http_async_thread(void *opaque)
{
	struct thread_args *ta = (struct thread_args *) opaque;

	bool ok = MTY_HttpRequest(ta->method, ta->headers[0] ? ta->headers : NULL, ta->secure,
		ta->host, ta->path, ta->body_len > 0 ? ta->body : NULL, ta->body_len, ta->timeout_ms,
		&ta->res_body, &ta->res_body_len, &ta->code);

	if (ok && ta->cb && ta->res_body && ta->res_body_len > 0)
		ta->cb(ta->code, &ta->res_body, &ta->res_body_len);

	ta->status = !ok ? MTY_ASYNC_ERROR : MTY_ASYNC_OK;
}

void MTY_HttpAsyncRequest(uint32_t *index, const char *method, const char *headers, bool secure,
	const char *host, const char *path, const void *body, size_t size, uint32_t timeout,
	MTY_HttpAsyncFunc func)
{
	if (*index != 0)
		MTY_ThreadPoolDetach(CTX->pool, *index, mty_http_async_free_ta);

	struct thread_args *ta = calloc(1, sizeof(struct thread_args));
	ta->secure = secure;
	ta->body_len = size;
	ta->timeout_ms = timeout;
	ta->cb = func;
	snprintf(ta->method, 32, "%s", method);
	snprintf(ta->host, 128, "%s", host);
	snprintf(ta->path, 1024, "%s", path);
	snprintf(ta->headers, 1024, "%s", headers ? headers : "");
	if (body)
		memcpy(ta->body, body, size);

	*index = MTY_ThreadPoolStart(CTX->pool, mty_http_async_thread, ta);

	if (*index == 0)
		free(ta);
}

MTY_Async MTY_HttpAsyncPoll(uint32_t index, void **response, size_t *size, uint16_t *status)
{
	if (index == 0)
		return MTY_ASYNC_DONE;

	struct thread_args *ta = NULL;
	MTY_Async r = MTY_ASYNC_DONE;
	MTY_ThreadState pstatus = MTY_ThreadPoolState(CTX->pool, index, (void **) &ta);

	if (pstatus == MTY_THREAD_STATE_DONE) {
		*response = ta->res_body;
		*size = ta->res_body_len;
		*status = ta->code;
		r = ta->status;

	} else if (pstatus == MTY_THREAD_STATE_RUNNING) {
		r = MTY_ASYNC_CONTINUE;
	}

	return r;
}

void MTY_HttpAsyncClear(uint32_t *index)
{
	MTY_ThreadPoolDetach(CTX->pool, *index, mty_http_async_free_ta);
	*index = 0;
}
