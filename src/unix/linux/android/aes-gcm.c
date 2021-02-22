// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>

#include "jnih.h"

struct MTY_AESGCM {
	jobject gcm;
	jobject key;

	uint8_t *ebuf[2];
	size_t esize;

	uint8_t *dbuf[2];
	size_t dsize;
};

#define AES_GCM_ENCRYPT 0x00000001
#define AES_GCM_DECRYPT 0x00000002

MTY_AESGCM *MTY_AESGCMCreate(const void *key)
{
	MTY_AESGCM *ctx = MTY_Alloc(1, sizeof(MTY_AESGCM));

	JNIEnv *env = MTY_JNIEnv();

	jstring jalg = jnih_strdup(env, "AES/GCM/NoPadding");
	ctx->gcm = jnih_static_obj(env, "javax/crypto/Cipher", "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Cipher;", jalg);

	jbyteArray jkey = jnih_dup(env, key, 16);
	jstring jalg_key = jnih_strdup(env, "AES");
	ctx->key = jnih_new(env, "javax/crypto/spec/SecretKeySpec", "([BLjava/lang/String;)V", jkey, jalg_key);

	jnih_retain(env, &ctx->gcm);
	jnih_retain(env, &ctx->key);

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

	if (size + 16 > ctx->esize) {
		ctx->esize = size + 16;
		ctx->ebuf[0] = MTY_Realloc(ctx->ebuf[0], size, 1);
		ctx->ebuf[1] = MTY_Realloc(ctx->ebuf[1], ctx->esize, 1);
	}

	memcpy(ctx->ebuf[0], plainText, size);

	jobject in = jnih_wrap(env, ctx->ebuf[0], size);
	jobject out = jnih_wrap(env, ctx->ebuf[1], size + 16);

	jbyteArray jnonce = jnih_dup(env, nonce, 12);
	jobject spec = jnih_new(env, "javax/crypto/spec/GCMParameterSpec", "(I[B)V", 128, jnonce);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	jnih_void(env, ctx->gcm, "init", "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V",
		AES_GCM_ENCRYPT, ctx->key, spec);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	jnih_int(env, ctx->gcm, "doFinal", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I", in, out);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	memcpy(cipherText, ctx->ebuf[1], size);
	memcpy(hash, ctx->ebuf[1] + size, 16);

	except:

	jnih_free(env, spec);
	jnih_free(env, jnonce);
	jnih_free(env, out);
	jnih_free(env, in);

	return r;
}

bool MTY_AESGCMDecrypt(MTY_AESGCM *ctx, const void *nonce, const void *cipherText, size_t size,
	const void *hash, void *plainText)
{
	bool r = true;
	JNIEnv *env = MTY_JNIEnv();

	if (size + 16 > ctx->dsize) {
		ctx->dsize = size + 16;
		ctx->dbuf[0] = MTY_Realloc(ctx->dbuf[0], ctx->dsize, 1);
		ctx->dbuf[1] = MTY_Realloc(ctx->dbuf[1], size, 1);
	}

	memcpy(ctx->dbuf[0], cipherText, size);
	memcpy(ctx->dbuf[0] + size, hash, 16);

	jobject in = jnih_wrap(env, ctx->dbuf[0], size + 16);
	jobject out = jnih_wrap(env, ctx->dbuf[1], size);

	jbyteArray jnonce = jnih_dup(env, nonce, 12);
	jobject spec = jnih_new(env, "javax/crypto/spec/GCMParameterSpec", "(I[B)V", 128, jnonce);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	jnih_void(env, ctx->gcm, "init", "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V",
		AES_GCM_DECRYPT, ctx->key, spec);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	jnih_int(env, ctx->gcm, "doFinal", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I", in, out);
	if (jnih_catch(env)) {
		r = false;
		goto except;
	}

	memcpy(plainText, ctx->dbuf[1], size);

	except:

	jnih_free(env, spec);
	jnih_free(env, jnonce);
	jnih_free(env, out);
	jnih_free(env, in);

	return r;
}

void MTY_AESGCMDestroy(MTY_AESGCM **aesgcm)
{
	if (!aesgcm || !*aesgcm)
		return;

	MTY_AESGCM *ctx = *aesgcm;

	JNIEnv *env = MTY_JNIEnv();

	for (uint8_t x = 0; x < 2; x++) {
		MTY_Free(ctx->ebuf[x]);
		MTY_Free(ctx->dbuf[x]);
	}

	jnih_release(env, &ctx->key);
	jnih_release(env, &ctx->gcm);

	MTY_Free(ctx);
	*aesgcm = NULL;
}
