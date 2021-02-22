// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "jnih.h"

#include <string.h>
#include <stdio.h>

void jnih_retain(JNIEnv *env, jobject *ref)
{
	jobject old = *ref;

	*ref = (*env)->NewGlobalRef(env, old);

	jnih_free(env, old);
}

void jnih_release(JNIEnv *env, jobject *ref)
{
	if (!ref || !*ref)
		return;

	(*env)->DeleteGlobalRef(env, *ref);
	*ref = NULL;
}

jbyteArray jnih_alloc(JNIEnv *env, size_t size)
{
	return (*env)->NewByteArray(env, size);
}

void jnih_free(JNIEnv *env, jobject ref)
{
	if (!ref)
		return;

	(*env)->DeleteLocalRef(env, ref);
}

void jnih_log(JNIEnv *env, jstring str)
{
	const char *cstr = (*env)->GetStringUTFChars(env, str, NULL);

	MTY_Log("%s", cstr);

	(*env)->ReleaseStringUTFChars(env, str, cstr);
}

void jnih_strcpy(JNIEnv *env, char *buf, size_t size, jstring str)
{
	const char *cstr = (*env)->GetStringUTFChars(env, str, NULL);

	snprintf(buf, size, "%s", cstr);

	(*env)->ReleaseStringUTFChars(env, str, cstr);
}

bool jnih_catch(JNIEnv *env)
{
	jthrowable ex = (*env)->ExceptionOccurred(env);

	if (ex) {
		(*env)->ExceptionClear(env);

		jstring jstr = jnih_obj(env, ex, "toString", "()Ljava/lang/String;");

		jnih_log(env, jstr);
		jnih_free(env, jstr);

		jnih_free(env, ex);

		return true;
	}

	return false;
}

jbyteArray jnih_dup(JNIEnv *env, const void *buf, size_t size)
{
	jbyteArray dup = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, dup, 0, size, buf);

	return dup;
}

jobject jnih_wrap(JNIEnv *env, void *buf, size_t size)
{
	return (*env)->NewDirectByteBuffer(env, buf, size);
}

jstring jnih_strdup(JNIEnv *env, const char *str)
{
	return (*env)->NewStringUTF(env, str);
}

void jnih_memcpy(JNIEnv *env, void *dst, jbyteArray jsrc, size_t size)
{
	jsize jsize = (*env)->GetArrayLength(env, jsrc);
	jbyte *src = (*env)->GetByteArrayElements(env, jsrc, NULL);

	memcpy(dst, src, (size_t) jsize < size ? (size_t) jsize : size);

	(*env)->ReleaseByteArrayElements(env, jsrc, src, 0);
}

jobject jnih_new(JNIEnv *env, const char *name, const char *sig, ...)
{
	va_list args;
	va_start(args, sig);

	jclass cls = (*env)->FindClass(env, name);
	jmethodID mid = (*env)->GetMethodID(env, cls, "<init>", sig);
	jobject obj = (*env)->NewObjectV(env, cls, mid, args);

	va_end(args);

	jnih_free(env, cls);

	return obj;
}

jobject jnih_static_obj(JNIEnv *env, const char *name, const char *func, const char *sig, ...)
{
	va_list args;
	va_start(args, sig);

	jclass cls = (*env)->FindClass(env, name);
	jmethodID mid = (*env)->GetStaticMethodID(env, cls, func, sig);
	jobject obj = (*env)->CallStaticObjectMethodV(env, cls, mid, args);

	va_end(args);

	jnih_free(env, cls);

	return obj;
}

void jnih_void(JNIEnv *env, jobject obj, const char *name, const char *sig, ...)
{
	va_list args;
	va_start(args, sig);

	jclass cls = (*env)->GetObjectClass(env, obj);

	jmethodID mid = (*env)->GetMethodID(env, cls, name, sig);
	(*env)->CallVoidMethodV(env, obj, mid, args);

	va_end(args);

	jnih_free(env, cls);
}

jobject jnih_obj(JNIEnv *env, jobject obj, const char *name, const char *sig, ...)
{
	va_list args;
	va_start(args, sig);

	jclass cls = (*env)->GetObjectClass(env, obj);
	jmethodID mid = (*env)->GetMethodID(env, cls, name, sig);
	jobject r = (*env)->CallObjectMethodV(env, obj, mid, args);

	va_end(args);

	jnih_free(env, cls);

	return r;
}

jint jnih_int(JNIEnv *env, jobject obj, const char *name, const char *sig, ...)
{
	va_list args;
	va_start(args, sig);

	jclass cls = (*env)->GetObjectClass(env, obj);
	jmethodID mid = (*env)->GetMethodID(env, cls, name, sig);
	jint r = (*env)->CallIntMethodV(env, obj, mid, args);

	va_end(args);

	jnih_free(env, cls);

	return r;
}
