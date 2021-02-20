// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ssl-dl.h"

struct MTY_Cert {
	char cn[64];
	X509 *cert;
	RSA *key;
};

struct MTY_TLS {
	char *fp;

	SSL *ssl;
	SSL_CTX *ctx;
	BIO *bio_in;
	BIO *bio_out;
};

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


// CACert store

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


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	if (!ssl_dl_global_init())
		return NULL;

	MTY_Cert *cert = MTY_Alloc(1, sizeof(MTY_Cert));

	BIGNUM *bne = BN_new();
	BN_set_word(bne, RSA_F4);

	cert->key = RSA_new();
	RSA_generate_key_ex(cert->key, 1024, bne, NULL);
	BN_free(bne);

	cert->cert = X509_new();
	X509_set_version(cert->cert, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(cert->cert), MTY_RandomUInt(1000000000, 2000000000));
	X509_gmtime_adj(X509_getm_notBefore(cert->cert), -24 * 3600);    // 1 day ago
	X509_gmtime_adj(X509_getm_notAfter(cert->cert), 30 * 24 * 3600); // 30 days out

	uint8_t rand_name[16];
	MTY_RandomBytes(rand_name, 16);
	MTY_BytesToHex(rand_name, 16, cert->cn, 64);

	X509_NAME *x509_name = X509_get_subject_name(cert->cert);
	X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, (const unsigned char *) cert->cn, -1, -1, 0);
	X509_set_issuer_name(cert->cert, x509_name);

	EVP_PKEY *pKey = EVP_PKEY_new();
	EVP_PKEY_assign(pKey, EVP_PKEY_RSA, cert->key);
	X509_set_pubkey(cert->cert, pKey);
	X509_sign(cert->cert, pKey, EVP_sha256());

	return cert;
}

static void tls_x509_to_fingerprint(X509 *cert, char *fingerprint, size_t size)
{
	memset(fingerprint, 0, size);

	uint32_t len = 0;
	uint8_t buf[EVP_MAX_MD_SIZE];
	X509_digest(cert, EVP_sha256(), buf, &len);

	MTY_Strcat(fingerprint, size, "sha-256 ");

	for (size_t x = 0; x < len; x++) {
		char append[8];
		snprintf(append, 8, "%02X:", buf[x]);
		MTY_Strcat(fingerprint, size, append);
	}

	// Remove the trailing ':'
	fingerprint[strlen(fingerprint) - 1] = '\0';
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	tls_x509_to_fingerprint(ctx->cert, fingerprint, size);
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	if (ctx->key)
		RSA_free(ctx->key);

	if (ctx->cert)
		X509_free(ctx->cert);

	MTY_Free(ctx);
	*cert = NULL;
}


// TLS, DTLS

static int32_t dtls_verify(int32_t ok, X509_STORE_CTX *ctx)
{
	return 1;
}

MTY_TLS *MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu)
{
	bool ok = true;

	MTY_TLS *ctx = MTY_Alloc(1, sizeof(MTY_TLS));

	ctx->ctx = SSL_CTX_new(type == MTY_TLS_TYPE_DTLS ? DTLS_method() : TLS_method());

	ctx->ssl = SSL_new(ctx->ctx);
	if (!ctx->ssl) {
		MTY_Log("'SSL_new' failed");
		ok = false;
		goto except;
	}

	int32_t e = SSL_set_cipher_list(ctx->ssl, TLS_CIPHER_LIST);
	if (e != 1) {
		ok = false;
		goto except;
	}

	SSL_set_connect_state(ctx->ssl);

	if (type == MTY_TLS_TYPE_DTLS) {
		SSL_CTX_set_quiet_shutdown(ctx->ctx, 1);
		SSL_set_options(ctx->ssl, SSL_OP_NO_TICKET | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
		SSL_set_verify(ctx->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, dtls_verify);

		// This will tell openssl to create larger datagrams
		SSL_set_options(ctx->ssl, SSL_OP_NO_QUERY_MTU);
		SSL_ctrl(ctx->ssl, SSL_CTRL_SET_MTU, mtu, NULL);

	} else {
		// Global lock for internal OpenSSL reference count
		MTY_GlobalLock(&TLS_GLOCK);

		if (TLS_STORE)
			SSL_ctrl(ctx->ssl, SSL_CTRL_SET_VERIFY_CERT_STORE, 1, TLS_STORE);

		MTY_GlobalUnlock(&TLS_GLOCK);

		SSL_set_options(ctx->ssl, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
		SSL_set_verify(ctx->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
		SSL_set_verify_depth(ctx->ssl, TLS_VERIFY_DEPTH);
	}

	if (peerFingerprint)
		ctx->fp = MTY_Strdup(peerFingerprint);

	if (cert) {
		SSL_use_certificate(ctx->ssl, cert->cert);
		SSL_use_RSAPrivateKey(ctx->ssl, cert->key);
	}

	if (host) {
		X509_VERIFY_PARAM *param = SSL_get0_param(ctx->ssl);
		X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
		X509_VERIFY_PARAM_set1_host(param, host, 0);

		SSL_ctrl(ctx->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (char *) host);
	}

	ctx->bio_in = BIO_new(BIO_s_mem());
	ctx->bio_out = BIO_new(BIO_s_mem());
	SSL_set_bio(ctx->ssl, ctx->bio_in, ctx->bio_out);

	except:

	if (!ok)
		MTY_TLSDestroy(&ctx);

	return ctx;
}

void MTY_TLSDestroy(MTY_TLS **tls)
{
	if (!tls || !*tls)
		return;

	MTY_TLS *ctx = *tls;

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

	MTY_Free(ctx->fp);

	MTY_Free(ctx);
	*tls = NULL;
}

static bool tls_verify_peer_fingerprint(MTY_TLS *tls, const char *fingerprint)
{
	if (!fingerprint)
		return false;

	X509 *peer_cert = SSL_get_peer_certificate(tls->ssl);

	if (peer_cert) {
		char found[MTY_FINGERPRINT_MAX];
		tls_x509_to_fingerprint(peer_cert, found, MTY_FINGERPRINT_MAX);

		bool match = !strcmp(found, fingerprint);

		X509_free(peer_cert);

		return match;
	}

	return false;
}

MTY_Async MTY_TLSHandshake(MTY_TLS *ctx, const void *buf, size_t size, MTY_TLSWriteFunc writeFunc, void *opaque)
{
	// If we have incoming data add it to the state machine
	if (buf && size > 0) {
		int32_t n = BIO_write(ctx->bio_in, buf, (int32_t) size);

		if (n != (int32_t) size)
			return MTY_ASYNC_ERROR;
	}

	// Poll for response data
	while (true) {
		SSL_do_handshake(ctx->ssl);

		size_t pending = BIO_ctrl_pending(ctx->bio_out);

		if (pending > 0) {
			char *pbuf = MTY_Alloc(pending, 1);
			size_t psize = BIO_read(ctx->bio_out, pbuf, (int32_t) pending);

			if (psize != pending) {
				MTY_Free(pbuf);
				return MTY_ASYNC_ERROR;
			}

			writeFunc(pbuf, pending, opaque);
			MTY_Free(pbuf);

		} else {
			break;
		}
	}

	if (SSL_is_init_finished(ctx->ssl)) {
		if (ctx->fp)
			return tls_verify_peer_fingerprint(ctx, ctx->fp) ?
				MTY_ASYNC_OK : MTY_ASYNC_ERROR;

		return MTY_ASYNC_OK;
	}

	return MTY_ASYNC_CONTINUE;
}

bool MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	// Perform the encryption, outputs to bio_out
	int32_t n = SSL_write(ctx->ssl, in, (int32_t) inSize);

	// SSL_write threw an error with our input data
	if (n <= 0) {
		MTY_Log("'SSL_write' failed with error %d:%d", n, SSL_get_error(ctx->ssl, n));
		return false;
	}

	// Retrieve pending encrypted data
	size_t pending = BIO_ctrl_pending(ctx->bio_out);

	// There is no pending data in bio_out even though SSL_write succeeded
	if (pending <= 0) {
		MTY_Log("'BIO_ctrl_pending' with return value %d", (int32_t) pending);
		return false;
	}

	// The pending data exceeds the size of our output buffer
	if ((int32_t) pending > (int32_t) outSize) {
		MTY_Log("'BIO_ctrl_pending' with return value %d", (int32_t) pending);
		return false;
	}

	// Read the encrypted data from bio_out
	n = BIO_read(ctx->bio_out, out, (int32_t) pending);

	// The size of the encrypted data does not match BIO_ctrl_pending. Something is wrong
	if (n != (int32_t) pending) {
		MTY_Log("'BIO_read' failed with return value %d", n);
		return false;
	}

	*written = n;

	return true;
}

bool MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	// Fill bio_in with encrypted data
	int32_t n = BIO_write(ctx->bio_in, in, (int32_t) inSize);

	// The size of the encrypted data written does not match sie_in. Something is wrong
	if (n != (int32_t) inSize) {
		MTY_Log("'BIO_write' failed with return value %d", n);
		return false;
	}

	// Decrypt the data
	n = SSL_read(ctx->ssl, out, (int32_t) outSize);

	// If SSL_read succeeds, return the number of bytes placed in out, otherwise the ssl error
	if (n <= 0) {
		MTY_Log("'SSL_read' failed with error %d:%d", n, SSL_get_error(ctx->ssl, n));
		return false;
	}

	*read = n;

	return true;
}


// 0x14   - Change Cipher Spec
// 0x16   - Handshake
// 0x17   - Application Data

// 0xFEFF - DTLS 1.0
// 0xFEFD - DTLS 1.2
// 0x0303 - TLS 1.2

bool MTY_TLSIsHandshake(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && (d[0] == 0x14 || d[0] == 0x16) && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}

bool MTY_TLSIsApplicationData(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && d[0] == 0x17 && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}
