// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <jni.h>

struct MTY_Cert {
	bool dummy;
};

struct MTY_TLS {
	char *fp;

	jobject context;
	jobject engine;
	jobject obuf;

	uint8_t *buf;
	size_t size;
	size_t offset;
};

#define TLS_BUF_SIZE (1024 * 1024)


// CACert store

bool MTY_HttpSetCACert(const char *cacert, size_t size)
{
	// Unnecessary, always use the Android's CACert store

	return false;
}


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

static bool tls_catch(JNIEnv *env)
{
	jthrowable ex = (*env)->ExceptionOccurred(env);
	if (ex) {
		(*env)->ExceptionClear(env);

		jclass cls = (*env)->FindClass(env, "java/lang/Object");
		jmethodID mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
		jstring jstr = (*env)->CallObjectMethod(env, ex, mid);

		const char *cstr = (*env)->GetStringUTFChars(env, jstr, NULL);
		MTY_Log("%s", cstr);

		(*env)->ReleaseStringUTFChars(env, jstr, cstr);

		return true;
	}

	return false;
}

MTY_TLS *MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu)
{
	MTY_TLS *ctx = MTY_Alloc(1, sizeof(MTY_TLS));

	JNIEnv *env = MTY_JNIEnv();

	// Context
	jclass cls = (*env)->FindClass(env, "javax/net/ssl/SSLContext");
	jmethodID mid = (*env)->GetStaticMethodID(env, cls, "getInstance", "(Ljava/lang/String;)Ljavax/net/ssl/SSLContext;");
	ctx->context = (*env)->CallStaticObjectMethod(env, cls, mid, (*env)->NewStringUTF(env, "TLSv1.2"));
	ctx->context = (*env)->NewGlobalRef(env, ctx->context);

	// Initialize context
	mid = (*env)->GetMethodID(env, cls, "init", "([Ljavax/net/ssl/KeyManager;[Ljavax/net/ssl/TrustManager;Ljava/security/SecureRandom;)V");
	(*env)->CallVoidMethod(env, ctx->context, mid, NULL, NULL, NULL);

	// Create engine, set hostname for verification
	mid = (*env)->GetMethodID(env, cls, "createSSLEngine", "(Ljava/lang/String;I)Ljavax/net/ssl/SSLEngine;");
	ctx->engine = (*env)->CallObjectMethod(env, ctx->context, mid, (*env)->NewStringUTF(env, host), 443);
	ctx->engine = (*env)->NewGlobalRef(env, ctx->engine);

	// Set client mode
	cls = (*env)->FindClass(env, "javax/net/ssl/SSLEngine");
	mid = (*env)->GetMethodID(env, cls, "setUseClientMode", "(Z)V");
	(*env)->CallVoidMethod(env, ctx->engine, mid, true);

	// Begin handshake
	mid = (*env)->GetMethodID(env, cls, "beginHandshake", "()V");
	(*env)->CallVoidMethod(env, ctx->engine, mid, true);

	// Preallocate bufffers
	cls = (*env)->FindClass(env, "java/nio/ByteBuffer");
	mid = (*env)->GetStaticMethodID(env, cls, "allocate", "(I)Ljava/nio/ByteBuffer;");
	ctx->obuf = (*env)->CallStaticObjectMethod(env, cls, mid, TLS_BUF_SIZE);
	ctx->obuf = (*env)->NewGlobalRef(env, ctx->obuf);

	if (peerFingerprint)
		ctx->fp = MTY_Strdup(peerFingerprint);

	return ctx;
}

void MTY_TLSDestroy(MTY_TLS **tls)
{
	if (!tls || !*tls)
		return;

	MTY_TLS *ctx = *tls;

	JNIEnv *env = MTY_JNIEnv();

	if (ctx->engine)
		(*env)->DeleteGlobalRef(env, ctx->engine);

	if (ctx->context)
		(*env)->DeleteGlobalRef(env, ctx->context);

	if (ctx->obuf)
		(*env)->DeleteGlobalRef(env, ctx->obuf);

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
	JNIEnv *env = MTY_JNIEnv();

	// If we have input data, add it to our internal buffer
	if (buf && size > 0)
		tls_add_data(ctx, buf, size);

	bool finished = false;

	for (bool cont = true; cont;) {
		// Get handshake status
		jclass cls = (*env)->GetObjectClass(env, ctx->engine);
		jmethodID mid = (*env)->GetMethodID(env, cls, "getHandshakeStatus", "()Ljavax/net/ssl/SSLEngineResult$HandshakeStatus;");
		jobject status = (*env)->CallObjectMethod(env, ctx->engine, mid);

		// Convert the java enum to a string
		cls = (*env)->GetObjectClass(env, status);
		mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
		jstring jstr = (*env)->CallObjectMethod(env, status, mid);
		const char *cstr = (*env)->GetStringUTFChars(env, jstr, NULL);

		// Wrap any input data in an ephemeral ByteBuffer
		jbyteArray ba = (*env)->NewByteArray(env, ctx->offset);
		(*env)->SetByteArrayRegion(env, ba, 0, ctx->offset, (jbyte *) ctx->buf);

		// Clear the output buffer, reset positions
		cls = (*env)->GetObjectClass(env, ctx->obuf);
		mid = (*env)->GetMethodID(env, cls, "clear", "()Ljava/nio/Buffer;");
		(*env)->CallObjectMethod(env, ctx->obuf, mid);

		// Wrap the internal input buffer in a java ByteBuffer
		cls = (*env)->FindClass(env, "java/nio/ByteBuffer");
		mid = (*env)->GetStaticMethodID(env, cls, "wrap", "([B)Ljava/nio/ByteBuffer;");
		jobject bb = (*env)->CallStaticObjectMethod(env, cls, mid, ba);

		jobject result = NULL;

		// The handshake produced outbound data
		if (!strcmp(cstr, "NEED_WRAP")) {
			cls = (*env)->GetObjectClass(env, ctx->engine);
			mid = (*env)->GetMethodID(env, cls, "wrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;");

			result = (*env)->CallObjectMethod(env, ctx->engine, mid, bb, ctx->obuf);
			if (tls_catch(env))
				return MTY_ASYNC_ERROR;

		// The handshake wants input data
		} else if (!strcmp(cstr, "NEED_UNWRAP")) {
			cls = (*env)->GetObjectClass(env, ctx->engine);
			mid = (*env)->GetMethodID(env, cls, "unwrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;");

			result = (*env)->CallObjectMethod(env, ctx->engine, mid, bb, ctx->obuf);
			if (tls_catch(env))
				return MTY_ASYNC_ERROR;

		// Unexpected handshake status
		} else {
			MTY_Log("Handshake in unexpected state '%s'", cstr);
			return MTY_ASYNC_ERROR;
		}

		(*env)->ReleaseStringUTFChars(env, jstr, cstr);

		// If any of our internal buffer has been consumed, adjust
		cls = (*env)->GetObjectClass(env, bb);
		mid = (*env)->GetMethodID(env, cls, "position", "()I");
		jint pos = (*env)->CallIntMethod(env, bb, mid);

		ctx->offset -= pos;
		memmove(ctx->buf, ctx->buf + pos, ctx->offset);

		// If any data has been written to the output buffer, send it via the callback
		cls = (*env)->GetObjectClass(env, ctx->obuf);
		mid = (*env)->GetMethodID(env, cls, "array", "()[B");
		ba = (*env)->CallObjectMethod(env, ctx->obuf, mid);
		jbyte *b = (*env)->GetByteArrayElements(env, ba, NULL);

		mid = (*env)->GetMethodID(env, cls, "position", "()I");
		jint ba_size = (*env)->CallIntMethod(env, ctx->obuf, mid);

		if (ba_size > 0)
			writeFunc(b, ba_size, opaque);

		// Get wrap/unwrap handshake status
		cls = (*env)->GetObjectClass(env, result);
		mid = (*env)->GetMethodID(env, cls, "getHandshakeStatus", "()Ljavax/net/ssl/SSLEngineResult$HandshakeStatus;");
		status = (*env)->CallObjectMethod(env, result, mid);

		// Convert the java enum to a string
		cls = (*env)->GetObjectClass(env, status);
		mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
		jstring rjstr = (*env)->CallObjectMethod(env, status, mid);
		const char *rcstr = (*env)->GetStringUTFChars(env, rjstr, NULL);
		bool repeat = !strcmp(rcstr, "NEED_WRAP");
		finished = !strcmp(rcstr, "FINISHED");
		(*env)->ReleaseStringUTFChars(env, rjstr, rcstr);

		if (!repeat)
			cont = false;
	}

	return finished ? MTY_ASYNC_OK : MTY_ASYNC_CONTINUE;
}

bool MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	JNIEnv *env = MTY_JNIEnv();

	jobject jin = (*env)->NewDirectByteBuffer(env, (void *) in, inSize);
	jobject jout = (*env)->NewDirectByteBuffer(env, out, outSize);

	jclass cls = (*env)->GetObjectClass(env, ctx->engine);
	jmethodID mid = (*env)->GetMethodID(env, cls, "wrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;");

	jobject result = (*env)->CallObjectMethod(env, ctx->engine, mid, jin, jout);
	if (tls_catch(env))
		return false;

	cls = (*env)->GetObjectClass(env, result);
	mid = (*env)->GetMethodID(env, cls, "bytesProduced", "()I");
	*written = (*env)->CallIntMethod(env, result, mid);

	return true;
}

bool MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	JNIEnv *env = MTY_JNIEnv();

	jobject jin = (*env)->NewDirectByteBuffer(env, (void *) in, inSize);
	jobject jout = (*env)->NewDirectByteBuffer(env, out, outSize);

	jclass cls = (*env)->GetObjectClass(env, ctx->engine);
	jmethodID mid = (*env)->GetMethodID(env, cls, "unwrap", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Ljavax/net/ssl/SSLEngineResult;");

	jobject result = (*env)->CallObjectMethod(env, ctx->engine, mid, jin, jout);
	if (tls_catch(env))
		return false;

	cls = (*env)->GetObjectClass(env, result);
	mid = (*env)->GetMethodID(env, cls, "bytesProduced", "()I");
	*read = (*env)->CallIntMethod(env, result, mid);

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
