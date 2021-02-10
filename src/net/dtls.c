#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ssl-dl.h"

struct mty_dtls_creds {
	char common_name[33];
	X509 *cert;
	RSA *key;
};

struct mty_dtls {
	SSL *ssl;
	BIO *bio_in;
	BIO *bio_out;
};

struct mty_dtlsx {
	MTY_Mutex *mutex;
	SSL_CTX *ctx;
};


// DTLS context

static int32_t mty_dtls_verify(int32_t ok, X509_STORE_CTX *ctx)
{
	return 1;
}

struct mty_dtlsx *mty_dtls_context_init(void)
{
	if (!ssl_dl_global_init())
		return NULL;

	struct mty_dtlsx *ctx = calloc(1, sizeof(struct mty_dtlsx));

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

	ctx->mutex = MTY_MutexCreate();

	return ctx;
}

void mty_dtls_context_destroy(struct mty_dtlsx **ctx_out)
{
	if (!ctx_out || !*ctx_out)
		return;

	struct mty_dtlsx *ctx = *ctx_out;

	MTY_MutexDestroy(&ctx->mutex);

	SSL_CTX_free(ctx->ctx);

	free(ctx);
	*ctx_out = NULL;
}


// Creds

struct mty_dtls_creds *mty_dtls_get_creds(void)
{
	if (!ssl_dl_global_init())
		return NULL;

	struct mty_dtls_creds *creds = calloc(1, sizeof(struct mty_dtls_creds));

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

static void mty_dtls_x509_to_fingerprint(X509 *cert, char *fingerprint)
{
	uint8_t buf[EVP_MAX_MD_SIZE];
	uint32_t len = 0;
	X509_digest(cert, EVP_sha256(), buf, &len);

	fingerprint[0] = '\0';
	const char *prepend = "sha-256 ";
	snprintf(fingerprint, MTY_NET_FINGERPRINT_SIZE, "%s", prepend);

	size_t offset = strlen(prepend);

	for (size_t x = 0; x < len; x++, offset += 3)
		snprintf(fingerprint + offset, MTY_NET_FINGERPRINT_SIZE - offset, "%02X:", buf[x]);

	//remove the trailing ':'
	fingerprint[offset - 1] = '\0';
}

void mty_dtls_get_fingerprint(struct mty_dtls_creds *creds, char *fingerprint)
{
	mty_dtls_x509_to_fingerprint(creds->cert, fingerprint);
}

void mty_dtls_free_creds(struct mty_dtls_creds *creds)
{
	if (!creds) return;

	if (creds->key)
		RSA_free(creds->key);

	if (creds->cert)
		X509_free(creds->cert);

	free(creds);
}


// Core

int32_t mty_dtls_init(struct mty_dtls **mty_dtls_in, struct mty_dtlsx *ctx, struct mty_dtls_creds *creds, bool should_accept, int32_t mtu)
{
	int32_t r = MTY_NET_OK;;

	struct mty_dtls *mty_dtls = *mty_dtls_in = calloc(1, sizeof(struct mty_dtls));

	MTY_MutexLock(ctx->mutex);

	mty_dtls->ssl = SSL_new(ctx->ctx);

	MTY_MutexUnlock(ctx->mutex);

	if (!mty_dtls->ssl) {r = MTY_NET_DTLS_ERR_SSL; MTY_Log("'SSL_new' failed"); goto except;}

	SSL_use_certificate(mty_dtls->ssl, creds->cert);
	SSL_use_RSAPrivateKey(mty_dtls->ssl, creds->key);

	mty_dtls->bio_in = BIO_new(BIO_s_mem());
	mty_dtls->bio_out = BIO_new(BIO_s_mem());
	SSL_set_bio(mty_dtls->ssl, mty_dtls->bio_in, mty_dtls->bio_out);

	if (should_accept) {
		SSL_set_accept_state(mty_dtls->ssl);
	} else {
		SSL_set_connect_state(mty_dtls->ssl);
	}

	//this will tell openssl to create larger datagrams
	SSL_set_options(mty_dtls->ssl, SSL_OP_NO_QUERY_MTU);
	SSL_ctrl(mty_dtls->ssl, SSL_CTRL_SET_MTU, mtu, NULL);

	except:

	if (r != MTY_NET_OK) {
		mty_dtls_close(mty_dtls);
		*mty_dtls_in = NULL;
	}

	return r;
}

void mty_dtls_close(struct mty_dtls *mty_dtls)
{
	if (!mty_dtls) return;

	if (mty_dtls->ssl) {
		SSL_shutdown(mty_dtls->ssl);
		SSL_free(mty_dtls->ssl);
	}

	free(mty_dtls);
}

static int32_t mty_dtls_verify_peer_fingerprint(struct mty_dtls *mty_dtls, char *fingerprint)
{
	X509 *peer_cert = SSL_get_peer_certificate(mty_dtls->ssl);

	if (peer_cert) {
		char found_fingerprint[MTY_NET_FINGERPRINT_SIZE];
		mty_dtls_x509_to_fingerprint(peer_cert, found_fingerprint);
		bool match = !strcmp(found_fingerprint, fingerprint);

		X509_free(peer_cert);

		return match ? MTY_NET_OK : MTY_NET_DTLS_ERR_CERT;
	}

	return MTY_NET_DTLS_ERR_CERT;
}

int32_t mty_dtls_handshake(struct mty_dtls *mty_dtls, char *buf, size_t buf_size, char *peer_fingerprint,
	int32_t (*write_cb)(char *buf, size_t len, void *opaque), void *opaque)
{
	//if we have incoming data add it to the state machine
	if (buf && buf_size > 0) {
		int32_t n = BIO_write(mty_dtls->bio_in, buf, (int32_t) buf_size);

		if (n != (int32_t) buf_size)
			return MTY_NET_DTLS_ERR_BIO_WRITE;
	}

	//poll for response data
	while (true) {
		SSL_do_handshake(mty_dtls->ssl);

		size_t pending = BIO_ctrl_pending(mty_dtls->bio_out);

		if (pending > 0) {
			char *pbuf = malloc(pending);
			size_t size = BIO_read(mty_dtls->bio_out, pbuf, (int32_t) pending);

			if (size != pending)
				return MTY_NET_DTLS_ERR_BUFFER;

			write_cb(pbuf, pending, opaque);
			free(pbuf);

		} else {
			break;
		}
	}

	if (SSL_is_init_finished(mty_dtls->ssl))
		return mty_dtls_verify_peer_fingerprint(mty_dtls, peer_fingerprint);

	return MTY_NET_WRN_CONTINUE;
}

int32_t mty_dtls_encrypt(struct mty_dtls *mty_dtls, char *buf_in, int32_t size_in, char *buf_out, int32_t size_out)
{
	//perform the encryption, outputs to bio_out
	int32_t n = SSL_write(mty_dtls->ssl, buf_in, size_in);

	//SSL_write threw an error with our input data
	if (n <= 0) {MTY_Log("SSL_write %d:%d", n, SSL_get_error(mty_dtls->ssl, n)); return MTY_NET_ERR_DEFAULT;}

	//retrieve pending encrypted data
	size_t pending = BIO_ctrl_pending(mty_dtls->bio_out);

	//there is no pending data in bio_out even though SSL_write succeeded
	if (pending <= 0) {MTY_Log("BIO_ctrl_pending %d", (int32_t) pending); return MTY_NET_DTLS_ERR_NO_DATA;}

	//the pending data exceeds the size of our output buffer
	if ((int32_t) pending > size_out) {MTY_Log("BIO_ctrl_pending %d", (int32_t) pending); return MTY_NET_DTLS_ERR_BUFFER;}

	//read the encrypted data from bio_out
	n = BIO_read(mty_dtls->bio_out, buf_out, (int32_t) pending);

	//the size of the encrypted data does not match BIO_ctrl_pending. Something is wrong
	if (n != (int32_t) pending) {MTY_Log("BIO_read %d", n); return MTY_NET_DTLS_ERR_BIO_READ;}

	//on success, the number of bytes placed into buf_out is returned
	return n;
}

int32_t mty_dtls_decrypt(struct mty_dtls *mty_dtls, char *buf_in, int32_t size_in, char *buf_out, int32_t size_out)
{
	//fill bio_in with encrypted data
	int32_t n = BIO_write(mty_dtls->bio_in, buf_in, size_in);

	//the size of the encrypted data written does not match sie_in. Something is wrong
	if (n != size_in) {MTY_Log("BIO_write %d", n); return MTY_NET_DTLS_ERR_BIO_WRITE;}

	//decrypt the data
	n = SSL_read(mty_dtls->ssl, buf_out, size_out);

	//if SSL_read succeeds, return the number of bytes placed in buf_out, otherwise the ssl error
	if (n <= 0) {MTY_Log("SSL_read %d", n, SSL_get_error(mty_dtls->ssl, n)); return MTY_NET_ERR_DEFAULT;}

	return n;
}

int32_t mty_dtls_is_handshake(uint8_t *packet, int32_t n)
{
	//0x16feff == DTLS 1.2 Client Hello
	//0x16fefd == DTLS 1.2 Server Hello, Client Key Exchange, New Session Ticket
	if (n > 2 &&
		(packet[0] == 0x16 || packet[0] == 0x14) &&
		(packet[1] == 0xfe && (packet[2] == 0xfd || packet[2] == 0xff)))

		return MTY_NET_OK;

	return MTY_NET_ERR_DEFAULT;
}

int32_t mty_dtls_is_application_data(uint8_t *packet, int32_t n)
{
	//0x17fefd == DTLS 1.2 Application Data
	if (n > 2 &&
		packet[0] == 0x17 &&
		(packet[1] == 0xfe && packet[2] == 0xfd))

		return MTY_NET_OK;

	return MTY_NET_ERR_DEFAULT;
}
