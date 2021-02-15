// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "tls.h"

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

static char **tls_parse_cacert(const char *raw, size_t size, uint32_t *n)
{
	char *cacert = calloc(size + 1, 1);
	memcpy(cacert, raw, size);

	char **out = NULL;
	uint32_t out_len = 0;

	char *tok = cacert;
	char *next = strstr(tok, "\n\n");
	if (!next) next = strstr(tok, "\r\n\r\n");

	while (next) {
		out_len++;
		out = realloc(out, sizeof(char *) * out_len);

		size_t this_len = next - tok;
		out[out_len - 1] = calloc(this_len + 1, 1);
		memcpy(out[out_len - 1], tok, this_len);

		tok = next + 2;
		next = strstr(tok, "\n\n");
		if (!next) next = strstr(tok, "\r\n\r\n");
	}

	free(cacert);

	*n = out_len;
	return out;
}

bool tls_load_cacert(const char *cacert, size_t size)
{
	bool was_set = false;

	if (!ssl_dl_global_init())
		return MTY_NET_TLS_ERR_CONTEXT;

	uint32_t num_certs = 0;
	char **parsed_cacert = NULL;

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

	parsed_cacert = tls_parse_cacert(cacert, size, &num_certs);

	for (uint32_t x = 0; x < num_certs; x++) {
		BIO *bio = BIO_new_mem_buf(parsed_cacert[x], -1);

		if (bio) {
			X509 *cert = NULL;

			if (PEM_read_bio_X509(bio, &cert, 0, NULL)) {
				was_set = true;
				X509_STORE_add_cert(TLS_STORE, cert);
				X509_free(cert);

			} else {
				was_set = false;
				MTY_Log("'PEM_read_bio_X509' failed");
			}

			BIO_free(bio);
		}

		if (!was_set) {
			X509_STORE_free(TLS_STORE);
			TLS_STORE = NULL;
			break;
		}
	}

	except:

	for (uint32_t x = 0; x < num_certs; x++)
		free(parsed_cacert[x]);

	free(parsed_cacert);

	MTY_GlobalUnlock(&TLS_GLOCK);

	return was_set;
}


// Context

struct tls_context {
	TCP_SOCKET socket;
	SSL_CTX *ctx;
	SSL *ssl;
};

void tls_close(struct tls_context *tls)
{
	if (!tls) return;

	if (tls->ssl) {
		int32_t e;

		//SSL_shutdown may need to be called twice
		e = SSL_shutdown(tls->ssl);
		if (e == 0)
			SSL_shutdown(tls->ssl);

		MTY_GlobalLock(&TLS_GLOCK);

		SSL_free(tls->ssl);

		MTY_GlobalUnlock(&TLS_GLOCK);
	}

	if (tls->ctx)
		SSL_CTX_free(tls->ctx);

	free(tls);
}

static int32_t tls_context_new(struct tls_context **tls_in, TCP_SOCKET socket)
{
	struct tls_context *tls = *tls_in = calloc(1, sizeof(struct tls_context));

	if (!ssl_dl_global_init()) return MTY_NET_TLS_ERR_CONTEXT;

	//keep handle to the underlying tcp_context
	tls->socket = socket;

	//the SSL context can be reused for multiple connections
	tls->ctx = SSL_CTX_new(TLS_method());
	if (!tls->ctx) return MTY_NET_TLS_ERR_CONTEXT;

	//the SSL instance
	tls->ssl = SSL_new(tls->ctx);
	if (!tls->ssl) return MTY_NET_TLS_ERR_SSL;

	//cacert store
	MTY_GlobalLock(&TLS_GLOCK);

	if (TLS_STORE)
		SSL_ctrl(tls->ssl, SSL_CTRL_SET_VERIFY_CERT_STORE, 1, TLS_STORE);

	MTY_GlobalUnlock(&TLS_GLOCK);

	//disable any non TLS protocols
	long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
	SSL_set_options(tls->ssl, flags);

	//limit ciphers to predefined secure list
	int32_t e = SSL_set_cipher_list(tls->ssl, TLS_CIPHER_LIST);
	if (e != 1) return MTY_NET_TLS_ERR_CIPHER;

	e = SSL_set_fd(tls->ssl, socket);
	if (e != 1) return MTY_NET_TLS_ERR_FD;

	return MTY_NET_TLS_OK;
}

static int32_t tls_handshake_poll(struct tls_context *tls, int32_t e, int32_t timeout_ms)
{
	int32_t ne = tcp_error();
	if (ne == tcp_bad_fd()) return MTY_NET_TLS_ERR_FD;

	if (ne == tcp_would_block() || SSL_get_error(tls->ssl, e) == SSL_ERROR_WANT_READ)
		return tcp_poll(tls->socket, TCP_POLLIN, timeout_ms);

	return MTY_NET_TLS_ERR_HANDSHAKE;
}

int32_t tls_connect(struct tls_context **tls_in, TCP_SOCKET socket,
	const char *host, bool verify_host, int32_t timeout_ms)
{
	int32_t r = MTY_NET_TLS_OK;

	int32_t e = tls_context_new(tls_in, socket);
	struct tls_context *tls = *tls_in;
	if (e != MTY_NET_TLS_OK) {r = e; goto except;}

	//set peer certificate verification
	SSL_set_verify(tls->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify_depth(tls->ssl, TLS_VERIFY_DEPTH);

	//set hostname validation
	if (verify_host) {
		X509_VERIFY_PARAM *param = SSL_get0_param(tls->ssl);
		X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
		X509_VERIFY_PARAM_set1_host(param, host, 0);
	}

	//set hostname extension -- sometimes required
	SSL_ctrl(tls->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (char *) host);

	while (1) {
		//attempt SSL connection on nonblocking socket -- 1 is success
		e = SSL_connect(tls->ssl);
		if (e == 1) break;

		//if not successful, see if we neeed to poll for more data
		e = tls_handshake_poll(tls, e, timeout_ms);
		if (e != MTY_NET_TLS_OK) {r = e; break;}
	}

	except:

	if (r != MTY_NET_TLS_OK) {
		tls_close(tls);
		*tls_in = NULL;
	}

	return r;
}

int32_t tls_accept(struct tls_context **tls_in, TCP_SOCKET socket, int32_t timeout_ms)
{
	int32_t r = MTY_NET_TLS_OK;

	int32_t e = tls_context_new(tls_in, socket);
	struct tls_context *tls = *tls_in;
	if (e != MTY_NET_TLS_OK) {r = e; goto except;}

	while (1) {
		//attempt SSL accept on nonblocking socket -- 1 is success
		e = SSL_accept(tls->ssl);
		if (e == 1) break;

		//if not successful, see if we neeed to poll for more data
		e = tls_handshake_poll(tls, e, timeout_ms);
		if (e != MTY_NET_TLS_OK) {r = e; break;}
	}

	except:

	if (r != MTY_NET_TLS_OK) {
		tls_close(tls);
		*tls_in = NULL;
	}

	return r;
}

int32_t tls_write(void *ctx, const char *buf, size_t size)
{
	struct tls_context *tls = (struct tls_context *) ctx;

	for (size_t total = 0; total < size;) {
		int32_t n = SSL_write(tls->ssl, buf + total, (int32_t) (size - total));
		if (n <= 0) {
			int32_t ssl_e = SSL_get_error(tls->ssl, n);
			if (ssl_e == SSL_ERROR_WANT_READ || ssl_e == SSL_ERROR_WANT_WRITE) continue;
			if (ssl_e == SSL_ERROR_ZERO_RETURN) return MTY_NET_TLS_ERR_CLOSED;
			return MTY_NET_TLS_ERR_WRITE;
		}

		total += n;
	}

	return MTY_NET_TLS_OK;
}

int32_t tls_read(void *ctx, char *buf, size_t size, int32_t timeout_ms)
{
	struct tls_context *tls = (struct tls_context *) ctx;

	for (size_t total = 0; total < size;) {
		if (SSL_has_pending(tls->ssl) == 0) {
			int32_t e = tcp_poll(tls->socket, TCP_POLLIN, timeout_ms);
			if (e != MTY_NET_TLS_OK) return e;
		}

		int32_t n = SSL_read(tls->ssl, buf + total, (int32_t) (size - total));
		if (n <= 0) {
			int32_t ssl_e = SSL_get_error(tls->ssl, n);
			if (ssl_e == SSL_ERROR_WANT_READ || ssl_e == SSL_ERROR_WANT_WRITE) continue;
			if (ssl_e == SSL_ERROR_ZERO_RETURN) return MTY_NET_TLS_ERR_CLOSED;
			return MTY_NET_TLS_ERR_READ;
		}

		total += n;
	}

	return MTY_NET_TLS_OK;
}
