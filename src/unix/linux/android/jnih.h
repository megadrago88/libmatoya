// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <jni.h>

void jnih_free(JNIEnv *env, jobject ref);
void jnih_retain(JNIEnv *env, jobject *ref);
void jnih_release(JNIEnv *env, jobject *ref);

jbyteArray jnih_alloc(JNIEnv *env, size_t size);
jbyteArray jnih_dup(JNIEnv *env, const void *buf, size_t size);
jobject jnih_wrap(JNIEnv *env, void *buf, size_t size);
jstring jnih_strdup(JNIEnv *env, const char *str);

void jnih_log(JNIEnv *env, jstring str);
bool jnih_catch(JNIEnv *env);

void jnih_memcpy(JNIEnv *env, void *dst, jbyteArray jsrc, size_t size);
void jnih_strcpy(JNIEnv *env, char *buf, size_t size, jstring str);

jobject jnih_new(JNIEnv *env, const char *name, const char *sig, ...);

jobject jnih_static_obj(JNIEnv *env, const char *name, const char *func, const char *sig, ...);

void jnih_void(JNIEnv *env, jobject obj, const char *name, const char *sig, ...);
jobject jnih_obj(JNIEnv *env, jobject obj, const char *name, const char *sig, ...);
jint jnih_int(JNIEnv *env, jobject obj, const char *name, const char *sig, ...);
