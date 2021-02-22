// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>

#include "jnih.h"

// Hash

static void crypto_hash_hmac(const char *alg, const void *input, size_t inputSize, const void *key,
	size_t keySize, void *output, size_t outputSize)
{
	JNIEnv *env = MTY_JNIEnv();

	jstring jalg = jnih_strdup(env, alg);
	jbyteArray jkey = jnih_dup(env, key, keySize);
	jbyteArray jin = jnih_dup(env, input, inputSize);

	jobject okey = jnih_new(env, "javax/crypto/spec/SecretKeySpec", "([BLjava/lang/String;)V", jkey, jalg);
	jobject omac = jnih_static_obj(env, "javax/crypto/Mac", "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Mac;", jalg);

	jnih_void(env, omac, "init", "(Ljava/security/Key;)V", okey);
	jbyteArray jout = jnih_obj(env, omac, "doFinal", "([B)[B", jin);

	jnih_memcpy(env, output, jout, outputSize);

	jnih_free(env, jout);
	jnih_free(env, omac);
	jnih_free(env, okey);
	jnih_free(env, jin);
	jnih_free(env, jkey);
	jnih_free(env, jalg);
}

static void crypto_hash(const char *alg, const void *input, size_t inputSize, void *output, size_t outputSize)
{
	JNIEnv *env = MTY_JNIEnv();

	jobject bb = jnih_wrap(env, (void *) input, inputSize);
	jstring jalg = jnih_strdup(env, alg);

	jobject obj = jnih_static_obj(env, "java/security/MessageDigest", "getInstance", "(Ljava/lang/String;)Ljava/security/MessageDigest;", jalg);

	jnih_void(env, obj, "update", "(Ljava/nio/ByteBuffer;)V", bb);
	jobject b = jnih_obj(env, obj, "digest", "()[B");

	jnih_memcpy(env, output, b, outputSize);

	jnih_free(env, b);
	jnih_free(env, obj);
	jnih_free(env, jalg);
	jnih_free(env, bb);
}

void MTY_CryptoHash(MTY_Algorithm algo, const void *input, size_t inputSize, const void *key,
	size_t keySize, void *output, size_t outputSize)
{
	switch (algo) {
		case MTY_ALGORITHM_SHA1:
			if (key && keySize > 0) {
				crypto_hash_hmac("HmacSHA1", input, inputSize, key, keySize, output, outputSize);
			} else {
				crypto_hash("SHA-1", input, inputSize, output, outputSize);
			}
			break;
		case MTY_ALGORITHM_SHA1_HEX: {
			uint8_t bytes[MTY_SHA1_SIZE];
			MTY_CryptoHash(MTY_ALGORITHM_SHA1, input, inputSize, key, keySize, bytes, MTY_SHA1_SIZE);
			MTY_BytesToHex(bytes, sizeof(bytes), output, outputSize);
			break;
		}
		case MTY_ALGORITHM_SHA256:
			if (key && keySize > 0) {
				crypto_hash_hmac("HmacSHA256", input, inputSize, key, keySize, output, outputSize);
			} else {
				crypto_hash("SHA-256", input, inputSize, output, outputSize);
			}
			break;
		case MTY_ALGORITHM_SHA256_HEX: {
			uint8_t bytes[MTY_SHA256_SIZE];
			MTY_CryptoHash(MTY_ALGORITHM_SHA256, input, inputSize, key, keySize, bytes, MTY_SHA256_SIZE);
			MTY_BytesToHex(bytes, sizeof(bytes), output, outputSize);
			break;
		}
	}
}


// Random

void MTY_RandomBytes(void *output, size_t size)
{
	JNIEnv *env = MTY_JNIEnv();

	jbyteArray b = jnih_alloc(env, size);

	jobject obj = jnih_new(env, "java/security/SecureRandom", "()V");
	jnih_void(env, obj, "nextBytes", "([B)V", b);
	jnih_memcpy(env, output, b, size);

	jnih_free(env, obj);
	jnih_free(env, b);
}
