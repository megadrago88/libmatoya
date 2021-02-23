// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>

#include "jnih.h"

#define AES_GCM_ENCRYPT 0x00000001
#define AES_GCM_DECRYPT 0x00000002

#define AES_GCM_NUM_BUFS 6
#define AES_GCM_MAX      (8 * 1024)

struct MTY_AESGCM {
	jobject gcm;
	jobject key;

	jclass cls_gps;
	jclass cls_cipher;

	jmethodID m_gps_constructor;
	jmethodID m_cipher_init;
	jmethodID m_cipher_do_final;

	jbyteArray buf[AES_GCM_NUM_BUFS];
};

MTY_AESGCM *MTY_AESGCMCreate(const void *key)
{
	MTY_AESGCM *ctx = MTY_Alloc(1, sizeof(MTY_AESGCM));

	JNIEnv *env = MTY_JNIEnv();

	// The cipher handle
	jstring jalg = jnih_strdup(env, "AES/GCM/NoPadding");
	ctx->gcm = jnih_static_obj(env, "javax/crypto/Cipher", "getInstance",
		"(Ljava/lang/String;)Ljavax/crypto/Cipher;", jalg);
	jnih_retain(env, &ctx->gcm);

	// 128 bit AES key
	jbyteArray jkey = jnih_dup(env, key, 16);
	jstring jalg_key = jnih_strdup(env, "AES");
	ctx->key = jnih_new(env, "javax/crypto/spec/SecretKeySpec", "([BLjava/lang/String;)V", jkey, jalg_key);
	jnih_retain(env, &ctx->key);

	// Preload classes for performance, these need global references so the method IDs are valid
	ctx->cls_gps = (*env)->FindClass(env, "javax/crypto/spec/GCMParameterSpec");
	jnih_retain(env, &ctx->cls_gps);

	ctx->cls_cipher = (*env)->FindClass(env, "javax/crypto/Cipher");
	jnih_retain(env, &ctx->cls_cipher);

	// Preload methods for performance
	ctx->m_gps_constructor = (*env)->GetMethodID(env, ctx->cls_gps, "<init>", "(I[BII)V");
	ctx->m_cipher_init = (*env)->GetMethodID(env, ctx->cls_cipher, "init",
		"(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V");

	ctx->m_cipher_do_final = (*env)->GetMethodID(env, ctx->cls_cipher, "doFinal", "([BII[B)I");

	// Preallocate byte buffers
	for (uint8_t x = 0; x < AES_GCM_NUM_BUFS; x++) {
		ctx->buf[x] = (*env)->NewByteArray(env, AES_GCM_MAX);
		jnih_retain(env, &ctx->buf[x]);
	}

	jnih_free(env, jalg_key);
	jnih_free(env, jkey);
	jnih_free(env, jalg);

	return ctx;
}

bool MTY_AESGCMEncrypt(MTY_AESGCM *ctx, const void *nonce, const void *plainText, size_t size,
	void *hash, void *cipherText)
{
	bool r = true;

	JNIEnv *env = MTY_JNIEnv();

	(*env)->SetByteArrayRegion(env, ctx->buf[0], 0, 12, nonce);
	(*env)->SetByteArrayRegion(env, ctx->buf[1], 0, size, plainText);

	jobject spec = (*env)->NewObject(env, ctx->cls_gps, ctx->m_gps_constructor, 128, ctx->buf[0], 0, 12);

	(*env)->CallVoidMethod(env, ctx->gcm, ctx->m_cipher_init, AES_GCM_ENCRYPT, ctx->key, spec);
	(*env)->CallIntMethod(env, ctx->gcm, ctx->m_cipher_do_final, ctx->buf[1], 0, size, ctx->buf[2]);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	(*env)->GetByteArrayRegion(env, ctx->buf[2], 0, size, cipherText);
	(*env)->GetByteArrayRegion(env, ctx->buf[2], size, 16, hash);

	except:

	jnih_free(env, spec);

	return r;
}

bool MTY_AESGCMDecrypt(MTY_AESGCM *ctx, const void *nonce, const void *cipherText, size_t size,
	const void *hash, void *plainText)
{
	bool r = true;

	JNIEnv *env = MTY_JNIEnv();

	(*env)->SetByteArrayRegion(env, ctx->buf[3], 0, 12, nonce);
	(*env)->SetByteArrayRegion(env, ctx->buf[4], 0, size, cipherText);
	(*env)->SetByteArrayRegion(env, ctx->buf[4], size, 16, hash);

	jobject spec = (*env)->NewObject(env, ctx->cls_gps, ctx->m_gps_constructor, 128, ctx->buf[3], 0, 12);

	(*env)->CallVoidMethod(env, ctx->gcm, ctx->m_cipher_init, AES_GCM_DECRYPT, ctx->key, spec);
	(*env)->CallIntMethod(env, ctx->gcm, ctx->m_cipher_do_final, ctx->buf[4], 0, size + 16, ctx->buf[5]);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	(*env)->GetByteArrayRegion(env, ctx->buf[5], 0, size, plainText);

	except:

	jnih_free(env, spec);

	return r;
}

void MTY_AESGCMDestroy(MTY_AESGCM **aesgcm)
{
	if (!aesgcm || !*aesgcm)
		return;

	MTY_AESGCM *ctx = *aesgcm;

	JNIEnv *env = MTY_JNIEnv();

	for (uint8_t x = 0; x < AES_GCM_NUM_BUFS; x++)
		jnih_release(env, &ctx->buf[x]);

	jnih_release(env, &ctx->cls_gps);
	jnih_release(env, &ctx->cls_cipher);
	jnih_release(env, &ctx->key);
	jnih_release(env, &ctx->gcm);

	MTY_Free(ctx);
	*aesgcm = NULL;
}
