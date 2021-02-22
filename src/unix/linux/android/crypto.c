// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>

#include <jni.h>

// Hash

static void crypto_hash_hmac(const char *alg, const void *input, size_t inputSize, const void *key,
	size_t keySize, void *output, size_t outputSize)
{
	JNIEnv *env = MTY_JNIEnv();

	jstring jalg = (*env)->NewStringUTF(env, alg);
	jbyteArray jkey = (*env)->NewByteArray(env, keySize);
	(*env)->SetByteArrayRegion(env, jkey, 0, keySize, key);

	jbyteArray jin = (*env)->NewByteArray(env, inputSize);
	(*env)->SetByteArrayRegion(env, jin, 0, inputSize, input);

	jclass cls = (*env)->FindClass(env, "javax/crypto/spec/SecretKeySpec");
	jmethodID mid = (*env)->GetMethodID(env, cls, "<init>", "([BLjava/lang/String;)V");
	jobject obj = (*env)->NewObject(env, cls, mid, jkey, jalg);

	jclass cls2 = (*env)->FindClass(env, "javax/crypto/Mac");
	mid = (*env)->GetStaticMethodID(env, cls2, "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Mac;");
	jobject obj2 = (*env)->CallStaticObjectMethod(env, cls2, mid, jalg);

	mid = (*env)->GetMethodID(env, cls2, "init", "(Ljava/security/Key;)V");
	(*env)->CallVoidMethod(env, cls2, mid, obj);

	mid = (*env)->GetMethodID(env, cls2, "doFinal", "([B)[B");
	jbyteArray jout = (*env)->CallObjectMethod(env, cls2, mid, jin);

	jsize out_size = (*env)->GetArrayLength(env, jout);
	jbyte *b = (*env)->GetByteArrayElements(env, jout, NULL);

	outputSize = (size_t) b_size < outputSize ? (size_t) b_size : outputSize;
	memcpy(output, jb, outputSize);

	(*env)->ReleaseByteArrayElements(env, b, jb, 0);

	(*env)->DeleteLocalRef(env, obj2);
	(*env)->DeleteLocalRef(env, cls2);
	(*env)->DeleteLocalRef(env, obj);
	(*env)->DeleteLocalRef(env, cls);
	(*env)->DeleteLocalRef(env, jkey);
	(*env)->DeleteLocalRef(env, jalg);
}

static void crypto_hash(const char *alg, const void *input, size_t inputSize, void *output, size_t outputSize)
{
	JNIEnv *env = MTY_JNIEnv();

	jobject bb_in = (*env)->NewDirectByteBuffer(env, (void *) input, inputSize);
	jstring jalg = (*env)->NewStringUTF(env, alg);

	jclass cls = (*env)->FindClass(env, "java/security/MessageDigest");
	jmethodID mid = (*env)->GetStaticMethodID(env, cls, "getInstance", "(Ljava/lang/String;)Ljava/security/MessageDigest;");
	jobject obj = (*env)->CallStaticObjectMethod(env, cls, mid, jalg);

	mid = (*env)->GetMethodID(env, cls, "update", "(Ljava/nio/ByteBuffer;)V");
	(*env)->CallVoidMethod(env, obj, mid, bb_in);

	mid = (*env)->GetMethodID(env, cls, "digest", "()[B");
	jobject b = (*env)->CallObjectMethod(env, obj, mid);

	jsize b_size = (*env)->GetArrayLength(env, b);
	jbyte *jb = (*env)->GetByteArrayElements(env, b, NULL);

	outputSize = (size_t) b_size < outputSize ? (size_t) b_size : outputSize;
	memcpy(output, jb, outputSize);

	(*env)->ReleaseByteArrayElements(env, b, jb, 0);
	(*env)->DeleteLocalRef(env, b);
	(*env)->DeleteLocalRef(env, obj);
	(*env)->DeleteLocalRef(env, cls);
	(*env)->DeleteLocalRef(env, jalg);
	(*env)->DeleteLocalRef(env, bb_in);
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

	jbyteArray b = (*env)->NewByteArray(env, size);

	jclass cls = (*env)->FindClass(env, "java/security/SecureRandom");
	jmethodID mid = (*env)->GetMethodID(env, cls, "<init>", "()V");
	jobject obj = (*env)->NewObject(env, cls, mid);

	mid = (*env)->GetMethodID(env, cls, "nextBytes", "([B)V");
	(*env)->CallVoidMethod(env, obj, mid, b);

	jbyte *jb = (*env)->GetByteArrayElements(env, b, NULL);
	memcpy(output, jb, size);

	(*env)->ReleaseByteArrayElements(env, b, jb, 0);
	(*env)->DeleteLocalRef(env, b);
	(*env)->DeleteLocalRef(env, obj);
	(*env)->DeleteLocalRef(env, cls);
}
