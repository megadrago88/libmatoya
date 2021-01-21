// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <aaudio/AAudio.h>

struct MTY_Audio {
	AAudioStreamBuilder *builder;
	AAudioStream *stream;
	int32_t last_underruns;
};

static void audio_error(AAudioStream *stream, void *userData, aaudio_result_t error)
{
	MTY_Log("'AAudioStream' error %d", error);
}

MTY_Audio *MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer)
{
	MTY_Audio *ctx = MTY_Alloc(1, sizeof(MTY_Audio));

	AAudio_createStreamBuilder(&ctx->builder);
	AAudioStreamBuilder_setDeviceId(ctx->builder, AAUDIO_UNSPECIFIED);
	AAudioStreamBuilder_setSampleRate(ctx->builder, sampleRate);
	AAudioStreamBuilder_setChannelCount(ctx->builder, 2);
	AAudioStreamBuilder_setFormat(ctx->builder, AAUDIO_FORMAT_PCM_I16);
	AAudioStreamBuilder_setPerformanceMode(ctx->builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
	AAudioStreamBuilder_setErrorCallback(ctx->builder, audio_error, NULL);

	AAudioStreamBuilder_openStream(ctx->builder, &ctx->stream);

	return ctx;
}

uint32_t MTY_AudioGetQueuedMs(MTY_Audio *ctx)
{
	return 0;
}

void MTY_AudioStop(MTY_Audio *ctx)
{
}

void MTY_AudioQueue(MTY_Audio *ctx, const int16_t *frames, uint32_t count)
{
	AAudioStream_write(ctx->stream, frames, count, 40000000); // 40ms

	int32_t underruns = AAudioStream_getXRunCount(ctx->stream);
	int32_t state = AAudioStream_getState(ctx->stream);

	if (underruns > ctx->last_underruns || state != AAUDIO_STREAM_STATE_STARTED) {
		AAudioStream_requestStop(ctx->stream);
		AAudioStream_requestStart(ctx->stream);
		ctx->last_underruns = underruns;
	}
}

void MTY_AudioDestroy(MTY_Audio **audio)
{
	if (!audio || !*audio)
		return;

	MTY_Audio *ctx = *audio;

	if (ctx->stream) {
		AAudioStream_requestStop(ctx->stream);
		AAudioStream_close(ctx->stream);
	}

	if (ctx->builder)
		AAudioStreamBuilder_delete(ctx->builder);

	MTY_Free(ctx);
	*audio = NULL;
}
