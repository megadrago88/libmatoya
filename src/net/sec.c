// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sec.h"

#include <stdlib.h>
#include <string.h>

#include "ssl-dl.h"

static MTY_Atomic32 TLS_GLOCK;
static X509_STORE *TLS_STORE;

#define TLS_VERIFY_DEPTH 4

#define TLS_CIPHER_LIST \
	"ECDHE-ECDSA-AES128-GCM-SHA256:" \
	"ECDHE-ECDSA-AES256-GCM-SHA384:" \
	"ECDHE-ECDSA-AES128-SHA:" \
	"ECDHE-ECDSA-AES256-SHA:" \
	"ECDHE-ECDSA-AES128-SHA256:" \
	"ECDHE-ECDSA-AES256-SHA384:" \
	"ECDHE-RSA-AES128-GCM-SHA256:" \
	"ECDHE-RSA-AES256-GCM-SHA384:" \
	"ECDHE-RSA-AES128-SHA:" \
	"ECDHE-RSA-AES256-SHA:" \
	"ECDHE-RSA-AES128-SHA256:" \
	"ECDHE-RSA-AES256-SHA384:" \
	"DHE-RSA-AES128-GCM-SHA256:" \
	"DHE-RSA-AES256-GCM-SHA384:" \
	"DHE-RSA-AES128-SHA:" \
	"DHE-RSA-AES256-SHA:" \
	"DHE-RSA-AES128-SHA256:" \
	"DHE-RSA-AES256-SHA256"


// State

static bool tls_add_cacert_to_store(X509_STORE *store, const void *cacert, size_t size)
{
	bool r = true;

	BIO *bio = BIO_new_mem_buf(cacert, (int32_t) size);

	if (bio) {
		X509 *cert = NULL;

		if (PEM_read_bio_X509(bio, &cert, 0, NULL)) {
			X509_STORE_add_cert(store, cert);
			X509_free(cert);

		} else {
			MTY_Log("'PEM_read_bio_X509' failed");
			r = false;
		}

		BIO_free(bio);
	}

	return r;
}

static bool tls_parse_cacert(X509_STORE *store, const char *raw, size_t size)
{
	bool r = true;

	char *cacert = MTY_Strdup(raw);

	char *tok = cacert;
	char *next = strstr(tok, "\n\n");
	if (!next)
		next = strstr(tok, "\r\n\r\n");

	while (next) {
		r = tls_add_cacert_to_store(store, tok, next - tok);
		if (!r)
			break;

		tok = next + 2;
		next = strstr(tok, "\n\n");
		if (!next)
			next = strstr(tok, "\r\n\r\n");
	}

	MTY_Free(cacert);

	return r;
}

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	if (!ssl_dl_global_init())
		return false;

	bool was_set = false;

	MTY_GlobalLock(&TLS_GLOCK);

	// CACert is set to NULL, remove existing
	if (!cacert) {
		if (TLS_STORE) {
			X509_STORE_free(TLS_STORE);
			TLS_STORE = NULL;
		}

		goto except;
	}

	// CACert is already set
	if (TLS_STORE)
		goto except;

	// Generate a new store
	TLS_STORE = X509_STORE_new();
	if (!TLS_STORE)
		goto except;

	was_set = tls_parse_cacert(TLS_STORE, cacert, size);

	if (!was_set) {
		X509_STORE_free(TLS_STORE);
		TLS_STORE = NULL;
	}

	except:

	MTY_GlobalUnlock(&TLS_GLOCK);

	return was_set;
}


// Context

struct tls {
	TCP_SOCKET socket;
	SSL_CTX *ctx;
	SSL *ssl;
};

void tls_destroy(struct tls **tls)
{
	if (!tls || !*tls)
		return;

	struct tls *ctx = *tls;

	if (ctx->ssl) {
		// SSL_shutdown may need to be called twice
		if (SSL_shutdown(ctx->ssl) == 0)
			SSL_shutdown(ctx->ssl);

		MTY_GlobalLock(&TLS_GLOCK);

		SSL_free(ctx->ssl);

		MTY_GlobalUnlock(&TLS_GLOCK);
	}

	if (ctx->ctx)
		SSL_CTX_free(ctx->ctx);

	MTY_Free(ctx);
	*tls = NULL;
}

static struct tls *tls_new(TCP_SOCKET socket)
{
	if (!ssl_dl_global_init())
		return NULL;

	bool r = true;

	struct tls *ctx = MTY_Alloc(1, sizeof(struct tls));
	ctx->socket = socket;

	ctx->ctx = SSL_CTX_new(TLS_method());
	if (!ctx->ctx) {
		r = false;
		goto except;
	}

	ctx->ssl = SSL_new(ctx->ctx);
	if (!ctx->ssl) {
		r = false;
		goto except;
	}

	// Global lock for internal OpenSSL reference count
	MTY_GlobalLock(&TLS_GLOCK);

	if (TLS_STORE)
		SSL_ctrl(ctx->ssl, SSL_CTRL_SET_VERIFY_CERT_STORE, 1, TLS_STORE);

	MTY_GlobalUnlock(&TLS_GLOCK);

	long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
	SSL_set_options(ctx->ssl, flags);

	int32_t e = SSL_set_cipher_list(ctx->ssl, TLS_CIPHER_LIST);
	if (e != 1) {
		r = false;
		goto except;
	}

	// Even though sockets on windows can be 64-bit, OpenSSL requires them as a 32-bit int
	e = SSL_set_fd(ctx->ssl, (int32_t) socket);
	if (e != 1) {
		r = false;
		goto except;
	}

	except:

	if (!r)
		tls_destroy(&ctx);

	return ctx;
}

static MTY_Async tls_handshake_poll(struct tls *tls, int32_t e, uint32_t timeout)
{
	MTY_Async a = tcp_async_state();

	if (a != MTY_ASYNC_ERROR && (a == MTY_ASYNC_CONTINUE || SSL_get_error(tls->ssl, e) == SSL_ERROR_WANT_READ))
		return tcp_poll(tls->socket, false, timeout);

	return MTY_ASYNC_ERROR;
}

struct tls *tls_connect(TCP_SOCKET socket, const char *host, uint32_t timeout)
{
	struct tls *ctx = tls_new(socket);
	if (!ctx)
		return NULL;

	bool r = true;

	SSL_set_verify(ctx->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify_depth(ctx->ssl, TLS_VERIFY_DEPTH);

	X509_VERIFY_PARAM *param = SSL_get0_param(ctx->ssl);
	X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
	X509_VERIFY_PARAM_set1_host(param, host, 0);

	SSL_ctrl(ctx->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (char *) host);

	while (r) {
		int32_t e = SSL_connect(ctx->ssl);
		if (e == 1)
			break;

		if (tls_handshake_poll(ctx, e, timeout) != MTY_ASYNC_OK)
			r = false;
	}

	if (!r)
		tls_destroy(&ctx);

	return ctx;
}

struct tls *tls_accept(TCP_SOCKET socket, uint32_t timeout)
{
	struct tls *ctx = tls_new(socket);
	if (!ctx)
		return NULL;

	bool r = true;

	while (r) {
		int32_t e = SSL_accept(ctx->ssl);
		if (e == 1)
			break;

		if (tls_handshake_poll(ctx, e, timeout) != MTY_ASYNC_OK)
			r = false;
	}

	if (!r)
		tls_destroy(&ctx);

	return ctx;
}

bool tls_write(struct tls *ctx, const void *buf, size_t size)
{
	for (size_t total = 0; total < size;) {
		int32_t n = SSL_write(ctx->ssl, (uint8_t *) buf + total, (int32_t) (size - total));

		if (n <= 0) {
			int32_t e = SSL_get_error(ctx->ssl, n);
			if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE)
				return false;

		} else {
			total += n;
		}
	}

	return true;
}

bool tls_read(struct tls *ctx, void *buf, size_t size, uint32_t timeout)
{
	for (size_t total = 0; total < size;) {
		if (SSL_has_pending(ctx->ssl) == 0)
			if (tcp_poll(ctx->socket, false, timeout) != MTY_ASYNC_OK)
				return false;

		int32_t n = SSL_read(ctx->ssl, (uint8_t *) buf + total, (int32_t) (size - total));

		if (n <= 0) {
			int32_t e = SSL_get_error(ctx->ssl, n);
			if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE)
				return false;

		} else {
			total += n;
		}
	}

	return true;
}
