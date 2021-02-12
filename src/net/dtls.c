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
	char common_name[33];
	X509 *cert;
	RSA *key;
};

struct MTY_DTLS {
	SSL *ssl;
	SSL_CTX *ctx;
	BIO *bio_in;
	BIO *bio_out;
};


// Creds

MTY_Cert *MTY_CertCreate(void)
{
	if (!ssl_dl_global_init())
		return NULL;

	MTY_Cert *creds = calloc(1, sizeof(MTY_Cert));

	BIGNUM *bne = BN_new();
	BN_set_word(bne, RSA_F4);

	creds->key = RSA_new();
	RSA_generate_key_ex(creds->key, 1024, bne, NULL);
	BN_free(bne);

	creds->cert = X509_new();
	X509_set_version(creds->cert, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(creds->cert), MTY_RandomUInt(1000000000, 2000000000));
	X509_gmtime_adj(X509_getm_notBefore(creds->cert), -24 * 3600);    //1 day ago
	X509_gmtime_adj(X509_getm_notAfter(creds->cert), 30 * 24 * 3600); //30 days out

	uint8_t rand_name[16];
	char common_name[33];
	MTY_RandomBytes(rand_name, 16);
	MTY_BytesToHex(rand_name, 16, common_name, 33);
	snprintf(creds->common_name, 33, "%s", common_name);

	X509_NAME *x509_name = X509_get_subject_name(creds->cert);
	X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, (const unsigned char *) creds->common_name, -1, -1, 0);
	X509_set_issuer_name(creds->cert, x509_name);

	EVP_PKEY *pKey = EVP_PKEY_new();
	EVP_PKEY_assign(pKey, EVP_PKEY_RSA, creds->key);
	X509_set_pubkey(creds->cert, pKey);
	X509_sign(creds->cert, pKey, EVP_sha256());

	return creds;
}

static void mty_dtls_x509_to_fingerprint(X509 *cert, char *fingerprint, size_t size)
{
	uint8_t buf[EVP_MAX_MD_SIZE];
	uint32_t len = 0;
	X509_digest(cert, EVP_sha256(), buf, &len);

	fingerprint[0] = '\0';
	const char *prepend = "sha-256 ";
	snprintf(fingerprint, size, "%s", prepend);

	size_t offset = strlen(prepend);

	for (size_t x = 0; x < len; x++, offset += 3)
		snprintf(fingerprint + offset, size - offset, "%02X:", buf[x]);

	//remove the trailing ':'
	fingerprint[offset - 1] = '\0';
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	mty_dtls_x509_to_fingerprint(ctx->cert, fingerprint, size);
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

	free(ctx);
	*cert = NULL;
}


// Core

static int32_t mty_dtls_verify(int32_t ok, X509_STORE_CTX *ctx)
{
	return 1;
}

MTY_DTLS *MTY_DTLSCreate(MTY_Cert *cert, bool server, uint32_t mtu)
{
	bool ok = true;

	MTY_DTLS *ctx = calloc(1, sizeof(MTY_DTLS));

	ctx->ctx = SSL_CTX_new(DTLS_method());
	SSL_CTX_set_quiet_shutdown(ctx->ctx, 1);
	SSL_CTX_set_options(ctx->ctx, SSL_OP_NO_TICKET | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, mty_dtls_verify);

	SSL_CTX_set_cipher_list(ctx->ctx,
		"ECDHE-ECDSA-AES128-GCM-SHA256:"
		"ECDHE-ECDSA-AES128-SHA:"
		"ECDHE-ECDSA-AES128-SHA256:"
		"ECDHE-RSA-AES128-GCM-SHA256:"
		"ECDHE-RSA-AES128-SHA:"
		"ECDHE-RSA-AES128-SHA256:"
		"DHE-RSA-AES128-GCM-SHA256:"
		"DHE-RSA-AES128-SHA:"
		"DHE-RSA-AES128-SHA256:"
	);

	ctx->ssl = SSL_new(ctx->ctx);

	if (!ctx->ssl) {
		ok = false;
		MTY_Log("'SSL_new' failed");
		goto except;
	}

	SSL_use_certificate(ctx->ssl, cert->cert);
	SSL_use_RSAPrivateKey(ctx->ssl, cert->key);

	ctx->bio_in = BIO_new(BIO_s_mem());
	ctx->bio_out = BIO_new(BIO_s_mem());
	SSL_set_bio(ctx->ssl, ctx->bio_in, ctx->bio_out);

	if (server) {
		SSL_set_accept_state(ctx->ssl);

	} else {
		SSL_set_connect_state(ctx->ssl);
	}

	//this will tell openssl to create larger datagrams
	SSL_set_options(ctx->ssl, SSL_OP_NO_QUERY_MTU);
	SSL_ctrl(ctx->ssl, SSL_CTRL_SET_MTU, mtu, NULL);

	except:

	if (!ok)
		MTY_DTLSDestroy(&ctx);

	return ctx;
}

void MTY_DTLSDestroy(MTY_DTLS **dtls)
{
	if (!dtls || !*dtls)
		return;

	MTY_DTLS *ctx = *dtls;

	if (ctx->ssl) {
		SSL_shutdown(ctx->ssl);
		SSL_free(ctx->ssl);
	}

	if (ctx->ctx)
		SSL_CTX_free(ctx->ctx);

	free(ctx);
	*dtls = NULL;
}

static bool mty_dtls_verify_peer_fingerprint(MTY_DTLS *mty_dtls, const char *fingerprint)
{
	X509 *peer_cert = SSL_get_peer_certificate(mty_dtls->ssl);

	if (peer_cert) {
		char found_fingerprint[MTY_FINGERPRINT_MAX];
		mty_dtls_x509_to_fingerprint(peer_cert, found_fingerprint, MTY_FINGERPRINT_MAX);
		bool match = !strcmp(found_fingerprint, fingerprint);

		X509_free(peer_cert);

		return match;
	}

	return false;
}

MTY_DTLSStatus MTY_DTLSHandshake(MTY_DTLS *ctx, const void *packet, size_t size, const char *fingerprint,
	MTY_DTLSWriteFunc writeFunc, void *opaque)
{
	//if we have incoming data add it to the state machine
	if (packet && size > 0) {
		int32_t n = BIO_write(ctx->bio_in, packet, (int32_t) size);

		if (n != (int32_t) size)
			return MTY_DTLS_STATUS_ERROR;
	}

	//poll for response data
	while (true) {
		SSL_do_handshake(ctx->ssl);

		size_t pending = BIO_ctrl_pending(ctx->bio_out);

		if (pending > 0) {
			char *pbuf = malloc(pending);
			size_t psize = BIO_read(ctx->bio_out, pbuf, (int32_t) pending);

			if (psize != pending)
				return MTY_DTLS_STATUS_ERROR;

			writeFunc(pbuf, pending, opaque);
			free(pbuf);

		} else {
			break;
		}
	}

	if (SSL_is_init_finished(ctx->ssl))
		return mty_dtls_verify_peer_fingerprint(ctx, fingerprint) ?
			MTY_DTLS_STATUS_DONE : MTY_DTLS_STATUS_ERROR;

	return MTY_DTLS_STATUS_CONTINUE;
}

bool MTY_DTLSEncrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	//perform the encryption, outputs to bio_out
	int32_t n = SSL_write(ctx->ssl, in, (int32_t) inSize);

	//SSL_write threw an error with our input data
	if (n <= 0) {
		MTY_Log("SSL_write %d:%d", n, SSL_get_error(ctx->ssl, n));
		return false;
	}

	//retrieve pending encrypted data
	size_t pending = BIO_ctrl_pending(ctx->bio_out);

	//there is no pending data in bio_out even though SSL_write succeeded
	if (pending <= 0) {
		MTY_Log("BIO_ctrl_pending %d", (int32_t) pending);
		return false;
	}

	//the pending data exceeds the size of our output buffer
	if ((int32_t) pending > (int32_t) outSize) {
		MTY_Log("BIO_ctrl_pending %d", (int32_t) pending);
		return false;
	}

	//read the encrypted data from bio_out
	n = BIO_read(ctx->bio_out, out, (int32_t) pending);

	//the size of the encrypted data does not match BIO_ctrl_pending. Something is wrong
	if (n != (int32_t) pending) {
		MTY_Log("BIO_read %d", n);
		return false;
	}

	//on success, the number of bytes placed into out is returned
	*written = n;

	return true;
}

bool MTY_DTLSDecrypt(MTY_DTLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	//fill bio_in with encrypted data
	int32_t n = BIO_write(ctx->bio_in, in, (int32_t) inSize);

	//the size of the encrypted data written does not match sie_in. Something is wrong
	if (n != (int32_t) inSize) {
		MTY_Log("BIO_write %d", n);
		return false;
	}

	//decrypt the data
	n = SSL_read(ctx->ssl, out, (int32_t) outSize);

	//if SSL_read succeeds, return the number of bytes placed in out, otherwise the ssl error
	if (n <= 0) {
		MTY_Log("SSL_read %d", n, SSL_get_error(ctx->ssl, n));
		return false;
	}

	*read = n;

	return true;
}

bool MTY_DTLSIsHandshake(const void *packet, size_t size)
{
	const uint8_t *d = packet;

	//0x16feff == DTLS 1.2 Client Hello
	//0x16fefd == DTLS 1.2 Server Hello, Client Key Exchange, New Session Ticket
	return size > 2 && (d[0] == 0x16 || d[0] == 0x14) && (d[1] == 0xfe && (d[2] == 0xfd || d[2] == 0xff));
}

bool MTY_DTLSIsApplicationData(const void *packet, size_t size)
{
	const uint8_t *d = packet;

	//0x17fefd == DTLS 1.2 Application Data
	return size > 2 && d[0] == 0x17 && (d[1] == 0xfe && d[2] == 0xfd);
}
