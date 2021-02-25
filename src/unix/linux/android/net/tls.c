// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "jnih.h"

struct MTY_Cert {
	bool dummy;
};

struct MTY_TLS {
	char *fp;

	jobject engine;

	uint8_t *obuf;
	uint8_t *buf;
	size_t size;
	size_t offset;
};

#define TLS_BUF_SIZE (32 * 1024)


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	return MTY_Alloc(1, sizeof(MTY_Cert));
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	memset(fingerprint, 0, size);
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	MTY_Free(ctx);
	*cert = NULL;
}


// TLS, DTLS

MTY_TLS *MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu)
{
	MTY_TLS *ctx = MTY_Alloc(1, sizeof(MTY_TLS));

	JNIEnv *env = MTY_JNIEnv();

	// Context
	jstring proto = mty_jni_strdup(env, "TLSv1.2");
	jobject context = mty_jni_static_obj(env, "javax/net/ssl/SSLContext", "getInstance", "(Ljava/lang/String;)Ljavax/net/ssl/SSLContext;", proto);

	// Initialize context
	mty_jni_void(env, context, "init", "([Ljavax/net/ssl/KeyManager;[Ljavax/net/ssl/TrustManager;Ljava/security/SecureRandom;)V", NULL, NULL, NULL);

	// Create engine, set hostname for verification
	jstring jhost = mty_jni_strdup(env, host);
	ctx->engine = mty_jni_obj(env, context, "createSSLEngine", "(Ljava/lang/String;I)Ljavax/net/ssl/SSLEngine;", jhost, 443);

	// Set client mode
	mty_jni_void(env, ctx->engine, "setUseClientMode", "(Z)V", true);

	// Begin handshake
	mty_jni_void(env, ctx->engine, "beginHandshake", "()V", true);

	// Preallocate bufffers
	ctx->obuf = MTY_Alloc(TLS_BUF_SIZE, 1);

	if (peerFingerprint)
		ctx->fp = MTY_Strdup(peerFingerprint);

	mty_jni_retain(env, &ctx->engine);
	mty_jni_free(env, jhost);
	mty_jni_free(env, context);
	mty_jni_free(env, proto);

	return ctx;
}

void MTY_TLSDestroy(MTY_TLS **tls)
{
	if (!tls || !*tls)
		return;

	MTY_TLS *ctx = *tls;

	JNIEnv *env = MTY_JNIEnv();

	mty_jni_release(env, &ctx->engine);

	MTY_Free(ctx->obuf);
	MTY_Free(ctx->buf);
	MTY_Free(ctx->fp);

	MTY_Free(ctx);
	*tls = NULL;
}

static void tls_add_data(MTY_TLS *ctx, const void *buf, size_t size)
{
	if (size + ctx->offset > ctx->size) {
		ctx->size = size + ctx->offset;
		ctx->buf = MTY_Realloc(ctx->buf, ctx->size, 1);
	}

	memcpy(ctx->buf + ctx->offset, buf, size);
	ctx->offset += size;
}

MTY_Async MTY_TLSHandshake(MTY_TLS *ctx, const void *buf, size_t size, MTY_TLSWriteFunc writeFunc, void *opaque)
{
	MTY_Async r = MTY_ASYNC_CONTINUE;

	JNIEnv *env = MTY_JNIEnv();

	// If we have input data, add it to our internal buffer
	if (buf && size > 0)
		tls_add_data(ctx, buf, size);

	// Wrap any input data in an ephemeral ByteBuffer
	jobject jin = mty_jni_wrap(env, ctx->buf, ctx->offset);
	jobject jout = mty_jni_wrap(env, ctx->obuf, TLS_BUF_SIZE);

	while (true) {
		// Get handshake status and convert to string
		char action[32];
		jobject status = mty_jni_obj(env, ctx->engine, "getHandshakeStatus", "()Ljavax/net/ssl/SSLEngineResult$HandshakeStatus;");
		jstring jstr = mty_jni_obj(env, status, "toString", "()Ljava/lang/String;");
		mty_jni_strcpy(env, action, 32, jstr);
		mty_jni_free(env, jstr);
		mty_jni_free(env, status);

		jobject result = NULL;

		// The handshake produced outbound data
		if (!strcmp(action, "NEED_WRAP")) {
			result = mty_jni_obj(env, ctx->engine, "wrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;", jin, jout);
			if (mty_jni_catch(env)) {
				r = MTY_ASYNC_ERROR;
				break;
			}

		// The handshake wants input data
		} else if (!strcmp(action, "NEED_UNWRAP")) {
			result = mty_jni_obj(env, ctx->engine, "unwrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;", jin, jout);
			if (mty_jni_catch(env)) {
				r = MTY_ASYNC_ERROR;
				break;
			}

		// Unexpected handshake status
		} else {
			MTY_Log("Handshake in unexpected state '%s'", action);
			r = MTY_ASYNC_ERROR;
			break;
		}

		// If any of our internal buffer has been consumed, adjust
		jint ipos = mty_jni_int(env, result, "bytesConsumed", "()I");
		ctx->offset -= ipos;
		memmove(ctx->buf, ctx->buf + ipos, ctx->offset);

		// If any data has been written to the output buffer, send it via the callback
		jint pos = mty_jni_int(env, result, "bytesProduced", "()I");
		if (pos > 0)
			writeFunc(ctx->obuf, pos, opaque);

		// Get wrap/unwrap handshake status and convert to string
		status = mty_jni_obj(env, result, "getHandshakeStatus", "()Ljavax/net/ssl/SSLEngineResult$HandshakeStatus;");
		jstr = mty_jni_obj(env, status, "toString", "()Ljava/lang/String;");
		mty_jni_strcpy(env, action, 32, jstr);
		mty_jni_free(env, jstr);
		mty_jni_free(env, status);
		mty_jni_free(env, result);

		if (!strcmp(action, "FINISHED")) {
			r = MTY_ASYNC_OK;
			break;
		}

		if (strcmp(action, "NEED_WRAP")) {
			r = MTY_ASYNC_CONTINUE;
			break;
		}
	}

	return r;
}

bool MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	bool r = true;
	JNIEnv *env = MTY_JNIEnv();

	jobject jin = mty_jni_wrap(env, (void *) in, inSize);
	jobject jout = mty_jni_wrap(env, out, outSize);

	jobject result = mty_jni_obj(env, ctx->engine, "wrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;", jin, jout);
	if (mty_jni_catch(env)) {
		r = false;
		goto except;
	}

	*written = mty_jni_int(env, result, "bytesProduced", "()I");

	except:

	mty_jni_free(env, result);
	mty_jni_free(env, jout);
	mty_jni_free(env, jin);

	return r;
}

bool MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	bool r = true;
	JNIEnv *env = MTY_JNIEnv();

	jobject jin = mty_jni_wrap(env, (void *) in, inSize);
	jobject jout = mty_jni_wrap(env, out, outSize);

	jobject result = mty_jni_obj(env, ctx->engine, "unwrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;", jin, jout);
	if (mty_jni_catch(env)) {
		r = false;
		goto except;
	}

	*read = mty_jni_int(env, result, "bytesProduced", "()I");

	except:

	mty_jni_free(env, result);
	mty_jni_free(env, jout);
	mty_jni_free(env, jin);

	return r;
}
