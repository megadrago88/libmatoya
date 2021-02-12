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
	enum mty_net_scheme scheme;
	char host[128];
	char path[1024];
	char headers[1024];
	char body[5 * 1024];
	uint32_t body_len;
	int32_t timeout_ms;

	// Response
	enum mty_async_status status;
	int32_t code;
	char *res_body;
	uint32_t  res_body_len;
	MTY_RES_CB cb;
};

static struct mty_http_async {
	bool proxy;
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

void mty_http_async_init(uint32_t num_threads, bool proxy)
{
	if (CTX)
		return;

	CTX = calloc(1, sizeof(struct mty_http_async));
	CTX->proxy = proxy;

	CTX->pool = MTY_ThreadPoolCreate(num_threads);
}

void mty_http_async_destroy(void)
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

	ta->code = mty_http_request(ta->method, ta->scheme, ta->host, ta->path,
		ta->headers[0] ? ta->headers : NULL, ta->body_len > 0 ? ta->body : NULL,
		ta->body_len, ta->timeout_ms, &ta->res_body, &ta->res_body_len, CTX->proxy);

	if (ta->cb && ta->res_body && ta->res_body_len > 0)
		ta->cb(ta->code, &ta->res_body, &ta->res_body_len);

	ta->status = (ta->code <= 0) ? MTY_ASYNC_ERROR : MTY_ASYNC_OK;
}

void mty_http_async(uint32_t *req, const char *method, enum mty_net_scheme scheme, const char *host, const char *path,
	const char *headers, const char *body, uint32_t body_len, int32_t timeout_ms, MTY_RES_CB res_cb)
{
	if (*req != 0)
		MTY_ThreadPoolDetach(CTX->pool, *req, mty_http_async_free_ta);

	struct thread_args *ta = calloc(1, sizeof(struct thread_args));
	ta->scheme = scheme;
	ta->body_len = body_len;
	ta->timeout_ms = timeout_ms;
	ta->cb = res_cb;
	snprintf(ta->method, 32, "%s", method);
	snprintf(ta->host, 128, "%s", host);
	snprintf(ta->path, 1024, "%s", path);
	snprintf(ta->headers, 1024, "%s", headers ? headers : "");
	if (body)
		memcpy(ta->body, body, body_len);

	*req = MTY_ThreadPoolStart(CTX->pool, mty_http_async_thread, ta);

	if (*req == 0)
		free(ta);
}

enum mty_async_status mty_http_async_poll(uint32_t req, int32_t *status_code,
	char **response, uint32_t *response_len)
{
	if (req == 0)
		return MTY_ASYNC_NO_REQUEST;

	struct thread_args *ta = NULL;
	enum mty_async_status r = MTY_ASYNC_NO_REQUEST;
	MTY_ThreadState pstatus = MTY_ThreadPoolState(CTX->pool, req, (void **) &ta);

	if (pstatus == MTY_THREAD_STATE_DONE) {
		*response = ta->res_body;
		*response_len = ta->res_body_len;
		*status_code = ta->code;
		r = ta->status;

	} else if (pstatus == MTY_THREAD_STATE_RUNNING) {
		r = MTY_ASYNC_WAITING;
	}

	return r;
}

void mty_http_async_clear(uint32_t *req)
{
	MTY_ThreadPoolDetach(CTX->pool, *req, mty_http_async_free_ta);
	*req = 0;
}
