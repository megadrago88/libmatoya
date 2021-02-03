// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <android/api-level.h>


// Interface

#define AAUDIO_UNSPECIFIED 0

enum {
	AAUDIO_CALLBACK_RESULT_CONTINUE = 0,
	AAUDIO_CALLBACK_RESULT_STOP,
};

enum {
	AAUDIO_PERFORMANCE_MODE_NONE = 10,
	AAUDIO_PERFORMANCE_MODE_POWER_SAVING,
	AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
};

enum {
	AAUDIO_FORMAT_INVALID = -1,
	AAUDIO_FORMAT_UNSPECIFIED = 0,
	AAUDIO_FORMAT_PCM_I16,
	AAUDIO_FORMAT_PCM_FLOAT
};

typedef struct AAudioStreamStruct AAudioStream;
typedef struct AAudioStreamBuilderStruct AAudioStreamBuilder;

typedef int32_t aaudio_result_t;
typedef int32_t aaudio_format_t;
typedef int32_t aaudio_performance_mode_t;
typedef int32_t aaudio_data_callback_result_t;

typedef void (*AAudioStream_errorCallback)(AAudioStream *stream, void *userData, aaudio_result_t error);
typedef aaudio_data_callback_result_t (*AAudioStream_dataCallback)(AAudioStream *stream, void *userData,
	void *audioData, int32_t numFrames);

static aaudio_result_t (*AAudio_createStreamBuilder)(AAudioStreamBuilder** builder);
static void (*AAudioStreamBuilder_setDeviceId)(AAudioStreamBuilder* builder, int32_t deviceId);
static void (*AAudioStreamBuilder_setSampleRate)(AAudioStreamBuilder* builder, int32_t sampleRate);
void (*AAudioStreamBuilder_setChannelCount)(AAudioStreamBuilder* builder, int32_t channelCount);
void (*AAudioStreamBuilder_setFormat)(AAudioStreamBuilder* builder, aaudio_format_t format);
void (*AAudioStreamBuilder_setPerformanceMode)(AAudioStreamBuilder* builder, aaudio_performance_mode_t mode);
void (*AAudioStreamBuilder_setErrorCallback)(AAudioStreamBuilder* builder, AAudioStream_errorCallback callback, void *userData);
void (*AAudioStreamBuilder_setDataCallback)(AAudioStreamBuilder* builder, AAudioStream_dataCallback callback, void *userData);
void (*AAudioStreamBuilder_setBufferCapacityInFrames)(AAudioStreamBuilder* builder, int32_t numFrames);
static aaudio_result_t (*AAudioStreamBuilder_openStream)(AAudioStreamBuilder* builder, AAudioStream** stream);
static aaudio_result_t (*AAudioStream_setBufferSizeInFrames)(AAudioStream* stream, int32_t numFrames);
static aaudio_result_t (*AAudioStream_requestStop)(AAudioStream* stream);
static aaudio_result_t (*AAudioStream_requestStart)(AAudioStream* stream);
static aaudio_result_t (*AAudioStream_close)(AAudioStream* stream);
static aaudio_result_t (*AAudioStreamBuilder_delete)(AAudioStreamBuilder* builder);


// Runtime open

static MTY_Atomic32 AAUDIO_DL_LOCK;
static MTY_SO *AAUDIO_DL_SO;
static bool AAUDIO_DL_INIT;

static void aaudio_dl_global_destroy(void)
{
	MTY_GlobalLock(&AAUDIO_DL_LOCK);

	MTY_SOUnload(&AAUDIO_DL_SO);
	AAUDIO_DL_INIT = false;

	MTY_GlobalUnlock(&AAUDIO_DL_LOCK);
}

static bool aaudio_dl_global_init(void)
{
	MTY_GlobalLock(&AAUDIO_DL_LOCK);

	if (!AAUDIO_DL_INIT) {
		bool r = true;

		if (android_get_device_api_level() < __ANDROID_API_O__) {
			r = false;
			goto except;
		}

		AAUDIO_DL_SO = MTY_SOLoad("libaaudio.so");
		if (!AAUDIO_DL_SO) {
			r = false;
			goto except;
		}

		#define LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		LOAD_SYM(AAUDIO_DL_SO, AAudio_createStreamBuilder);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setDeviceId);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setSampleRate);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setChannelCount);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setFormat);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setPerformanceMode);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setErrorCallback);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setDataCallback);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_setBufferCapacityInFrames);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_openStream);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStream_setBufferSizeInFrames);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStream_requestStop);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStream_requestStart);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStream_close);
		LOAD_SYM(AAUDIO_DL_SO, AAudioStreamBuilder_delete);

		except:

		if (!r)
			aaudio_dl_global_destroy();

		AAUDIO_DL_INIT = r;
	}

	MTY_GlobalUnlock(&AAUDIO_DL_LOCK);

	return AAUDIO_DL_INIT;
}
