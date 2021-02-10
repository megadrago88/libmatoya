// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "matoya.h"


// Interface

#if defined(MTY_SSL_EXTERNAL)

#define FP(sym) sym
#define STATIC

#else

#define FP(sym) (*sym)
#define STATIC static
#endif

#define SSL_ERROR_WANT_READ                           2
#define SSL_ERROR_WANT_WRITE                          3
#define SSL_ERROR_ZERO_RETURN                         6

#define SSL_CTRL_SET_MTU                              17
#define SSL_CTRL_SET_TLSEXT_HOSTNAME                  55
#define TLSEXT_NAMETYPE_host_name                     0

#define SSL_VERIFY_PEER                               0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT               0x02
#define SSL_OP_NO_SSLv2                               0x0
#define SSL_OP_NO_QUERY_MTU                           0x00001000U
#define SSL_OP_NO_TICKET                              0x00004000U
#define SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION 0x00010000U
#define SSL_OP_NO_COMPRESSION                         0x00020000U
#define SSL_OP_NO_SSLv3                               0x02000000U

#define NID_rsaEncryption                             6
#define EVP_PKEY_RSA                                  NID_rsaEncryption

#define MBSTRING_FLAG                                 0x1000
#define MBSTRING_ASC                                  (MBSTRING_FLAG | 1)
#define X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS          0x4
#define RSA_F4                                        0x10001L
#define EVP_MAX_MD_SIZE                               64

typedef struct rsa_st RSA;
typedef struct bio_st BIO;
typedef struct bignum_st BIGNUM;
typedef struct x509_st X509;
typedef struct x509_store_st X509_STORE;
typedef struct X509_name_st X509_NAME;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct X509_VERIFY_PARAM_st X509_VERIFY_PARAM;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct bn_gencb_st BN_GENCB;
typedef struct asn1_string_st ASN1_INTEGER;
typedef struct asn1_string_st ASN1_TIME;
typedef struct evp_md_st EVP_MD;
typedef struct bio_method_st BIO_METHOD;

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_st SSL;

typedef int (*SSL_verify_cb)(int preverify_ok, X509_STORE_CTX *x509_ctx);
typedef int pem_password_cb(char *buf, int size, int rwflag, void *userdata);


// TLS, HTTPS

STATIC BIO *FP(BIO_new_mem_buf)(const void *buf, int len);
STATIC int FP(BIO_free)(BIO *a);

STATIC X509 *FP(PEM_read_bio_X509)(BIO *bp, X509 **x, pem_password_cb *cb, void *u);
STATIC int FP(X509_STORE_add_cert)(X509_STORE *ctx, X509 *x);
STATIC int FP(X509_VERIFY_PARAM_set1_host)(X509_VERIFY_PARAM *param, const char *name, size_t namelen);
STATIC void FP(X509_VERIFY_PARAM_set_hostflags)(X509_VERIFY_PARAM *param, unsigned int flags);
STATIC void FP(X509_free)(X509 *a);

STATIC RSA *FP(PEM_read_bio_RSAPrivateKey)(BIO *bp, RSA **x, pem_password_cb *cb, void *u);
STATIC void FP(RSA_free)(RSA *r);

STATIC SSL *FP(SSL_new)(SSL_CTX *ctx);
STATIC void FP(SSL_free)(SSL *ssl);
STATIC int FP(SSL_has_pending)(const SSL *s);
STATIC int FP(SSL_accept)(SSL *ssl);
STATIC int FP(SSL_connect)(SSL *ssl);
STATIC int FP(SSL_set_fd)(SSL *s, int fd);
STATIC int FP(SSL_shutdown)(SSL *s);
STATIC int FP(SSL_read)(SSL *ssl, void *buf, int num);
STATIC int FP(SSL_write)(SSL *ssl, const void *buf, int num);
STATIC void FP(SSL_set_verify)(SSL *s, int mode, SSL_verify_cb callback);
STATIC void FP(SSL_set_verify_depth)(SSL *s, int depth);
STATIC int FP(SSL_get_error)(const SSL *s, int ret_code);
STATIC X509_VERIFY_PARAM *FP(SSL_get0_param)(SSL *ssl);
STATIC long FP(SSL_ctrl)(SSL *ssl, int cmd, long larg, void *parg);

STATIC const SSL_METHOD *FP(TLS_method)(void);
STATIC SSL_CTX *FP(SSL_CTX_new)(const SSL_METHOD *meth);
STATIC void FP(SSL_CTX_free)(SSL_CTX *);
STATIC int FP(SSL_CTX_use_certificate)(SSL_CTX *ctx, X509 *x);
STATIC X509_STORE *FP(SSL_CTX_get_cert_store)(const SSL_CTX *);
STATIC unsigned long FP(SSL_CTX_set_options)(SSL_CTX *ctx, unsigned long op);
STATIC int FP(SSL_CTX_set_cipher_list)(SSL_CTX *, const char *str);
STATIC int FP(SSL_CTX_check_private_key)(const SSL_CTX *ctx);
STATIC int FP(SSL_CTX_use_RSAPrivateKey)(SSL_CTX *ctx, RSA *rsa);


// DTLS

STATIC void FP(SSL_set_bio)(SSL *s, BIO *rbio, BIO *wbio);
STATIC void FP(SSL_set_accept_state)(SSL *s);
STATIC void FP(SSL_set_connect_state)(SSL *s);
STATIC int FP(SSL_is_init_finished)(SSL *s);
STATIC int FP(SSL_do_handshake)(SSL *s);
STATIC int FP(SSL_use_certificate)(SSL *ssl, X509 *x);
STATIC unsigned long FP(SSL_set_options)(SSL *s, unsigned long op);
STATIC X509 *FP(SSL_get_peer_certificate)(const SSL *s);
STATIC int FP(SSL_use_RSAPrivateKey)(SSL *ssl, RSA *rsa);

STATIC const FP(SSL_METHOD *DTLS_method)(void);
STATIC void FP(SSL_CTX_set_quiet_shutdown)(SSL_CTX *ctx, int mode);
STATIC void FP(SSL_CTX_set_verify)(SSL_CTX *ctx, int mode, SSL_verify_cb callback);

STATIC BIO *FP(BIO_new)(const BIO_METHOD *type);
STATIC const FP(BIO_METHOD *BIO_s_mem)(void);
STATIC int FP(BIO_write)(BIO *b, const void *data, int len);
STATIC size_t FP(BIO_ctrl_pending)(BIO *b);
STATIC int FP(BIO_read)(BIO *b, void *data, int len);

STATIC X509 *FP(X509_new)(void);
STATIC int FP(X509_set_pubkey)(X509 *x, EVP_PKEY *pkey);
STATIC int FP(X509_sign)(X509 *x, EVP_PKEY *pkey, const EVP_MD *md);
STATIC int FP(X509_digest)(const X509 *data, const EVP_MD *type, unsigned char *md, unsigned int *len);
STATIC ASN1_TIME *FP(X509_getm_notBefore)(const X509 *x);
STATIC ASN1_TIME *FP(X509_getm_notAfter)(const X509 *x);
STATIC int FP(X509_set_version)(X509 *x, long version);
STATIC int FP(X509_set_issuer_name)(X509 *x, X509_NAME *name);
STATIC X509_NAME *FP(X509_get_subject_name)(const X509 *a);
STATIC ASN1_INTEGER *FP(X509_get_serialNumber)(X509 *x);
STATIC ASN1_TIME *FP(X509_gmtime_adj)(ASN1_TIME *s, long adj);
STATIC int FP(X509_NAME_add_entry_by_txt)(X509_NAME *name, const char *field, int type,
	const unsigned char *bytes, int len, int loc, int set);

STATIC BIGNUM *FP(BN_new)(void);
STATIC void FP(BN_free)(BIGNUM *a);
STATIC int FP(BN_set_word)(BIGNUM *a, unsigned long long w);

STATIC EVP_PKEY *FP(EVP_PKEY_new)(void);
STATIC const FP(EVP_MD *EVP_sha256)(void);
STATIC int FP(EVP_PKEY_assign)(EVP_PKEY *pkey, int type, void *key);

STATIC RSA *FP(RSA_new)(void);
STATIC int FP(ASN1_INTEGER_set)(ASN1_INTEGER *a, long v);
STATIC int FP(RSA_generate_key_ex)(RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);


// Runtime open

#if defined(MTY_SSL_EXTERNAL)

#define ssl_dl_global_init() true
#define ssl_dl_global_destroy()

#else

static MTY_Atomic32 SSL_DL_LOCK;
static MTY_SO *SSL_DL_SO;
static bool SSL_DL_INIT;

static void ssl_dl_global_destroy(void)
{
	MTY_GlobalLock(&SSL_DL_LOCK);

	MTY_SOUnload(&SSL_DL_SO);
	SSL_DL_INIT = false;

	MTY_GlobalUnlock(&SSL_DL_LOCK);
}

static bool ssl_dl_global_init(void)
{
	MTY_GlobalLock(&SSL_DL_LOCK);

	if (!SSL_DL_INIT) {
		bool r = true;
		SSL_DL_SO = MTY_SOLoad("libssl.so.1.1");

		if (!SSL_DL_SO)
			SSL_DL_SO = MTY_SOLoad("libssl.so.1.0.0");

		if (!SSL_DL_SO) {
			r = false;
			goto except;
		}

		#define LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		LOAD_SYM(SSL_DL_SO, BIO_new_mem_buf);
		LOAD_SYM(SSL_DL_SO, BIO_free);

		LOAD_SYM(SSL_DL_SO, PEM_read_bio_X509);
		LOAD_SYM(SSL_DL_SO, X509_STORE_add_cert);
		LOAD_SYM(SSL_DL_SO, X509_VERIFY_PARAM_set1_host);
		LOAD_SYM(SSL_DL_SO, X509_VERIFY_PARAM_set_hostflags);
		LOAD_SYM(SSL_DL_SO, X509_free);

		LOAD_SYM(SSL_DL_SO, PEM_read_bio_RSAPrivateKey);
		LOAD_SYM(SSL_DL_SO, RSA_free);

		LOAD_SYM(SSL_DL_SO, SSL_new);
		LOAD_SYM(SSL_DL_SO, SSL_free);
		LOAD_SYM(SSL_DL_SO, SSL_has_pending);
		LOAD_SYM(SSL_DL_SO, SSL_accept);
		LOAD_SYM(SSL_DL_SO, SSL_connect);
		LOAD_SYM(SSL_DL_SO, SSL_set_fd);
		LOAD_SYM(SSL_DL_SO, SSL_shutdown);
		LOAD_SYM(SSL_DL_SO, SSL_read);
		LOAD_SYM(SSL_DL_SO, SSL_write);
		LOAD_SYM(SSL_DL_SO, SSL_set_verify);
		LOAD_SYM(SSL_DL_SO, SSL_set_verify_depth);
		LOAD_SYM(SSL_DL_SO, SSL_get_error);
		LOAD_SYM(SSL_DL_SO, SSL_get0_param);
		LOAD_SYM(SSL_DL_SO, SSL_ctrl);

		LOAD_SYM(SSL_DL_SO, TLS_method);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_new);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_free);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_use_certificate);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_get_cert_store);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_set_options);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_set_cipher_list);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_check_private_key);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_use_RSAPrivateKey);

		LOAD_SYM(SSL_DL_SO, SSL_set_bio);
		LOAD_SYM(SSL_DL_SO, SSL_set_accept_state);
		LOAD_SYM(SSL_DL_SO, SSL_set_connect_state);
		LOAD_SYM(SSL_DL_SO, SSL_is_init_finished);
		LOAD_SYM(SSL_DL_SO, SSL_do_handshake);
		LOAD_SYM(SSL_DL_SO, SSL_use_certificate);
		LOAD_SYM(SSL_DL_SO, SSL_set_options);
		LOAD_SYM(SSL_DL_SO, SSL_get_peer_certificate);
		LOAD_SYM(SSL_DL_SO, SSL_use_RSAPrivateKey);

		LOAD_SYM(SSL_DL_SO, DTLS_method);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_set_quiet_shutdown);
		LOAD_SYM(SSL_DL_SO, SSL_CTX_set_verify);

		LOAD_SYM(SSL_DL_SO, BIO_new);
		LOAD_SYM(SSL_DL_SO, BIO_s_mem);
		LOAD_SYM(SSL_DL_SO, BIO_write);
		LOAD_SYM(SSL_DL_SO, BIO_ctrl_pending);
		LOAD_SYM(SSL_DL_SO, BIO_read);

		LOAD_SYM(SSL_DL_SO, X509_new);
		LOAD_SYM(SSL_DL_SO, X509_set_pubkey);
		LOAD_SYM(SSL_DL_SO, X509_sign);
		LOAD_SYM(SSL_DL_SO, X509_digest);
		LOAD_SYM(SSL_DL_SO, X509_getm_notBefore);
		LOAD_SYM(SSL_DL_SO, X509_getm_notAfter);
		LOAD_SYM(SSL_DL_SO, X509_set_version);
		LOAD_SYM(SSL_DL_SO, X509_set_issuer_name);
		LOAD_SYM(SSL_DL_SO, X509_get_subject_name);
		LOAD_SYM(SSL_DL_SO, X509_get_serialNumber);
		LOAD_SYM(SSL_DL_SO, 509_gmtime_adj);
		LOAD_SYM(SSL_DL_SO, 509_NAME_add_entry_by_txt);

		LOAD_SYM(SSL_DL_SO, BN_new);
		LOAD_SYM(SSL_DL_SO, BN_free);
		LOAD_SYM(SSL_DL_SO, BN_set_word);

		LOAD_SYM(SSL_DL_SO, EVP_PKEY_new);
		LOAD_SYM(SSL_DL_SO, EVP_MD *EVP_sha256);
		LOAD_SYM(SSL_DL_SO, EVP_PKEY_assign);

		LOAD_SYM(SSL_DL_SO, RSA_new);
		LOAD_SYM(SSL_DL_SO, ASN1_INTEGER_set);
		LOAD_SYM(SSL_DL_SO, RSA_generate_key_ex);

		except:

		if (!r)
			ssl_dl_global_destroy();

		SSL_DL_INIT = r;
	}

	MTY_GlobalUnlock(&SSL_DL_LOCK);

	return SSL_DL_INIT;
}

#endif
