// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AudioToolbox/AudioToolbox.h>

#define AUDIO_CHANNELS    2
#define AUDIO_SAMPLE_SIZE sizeof(int16_t)

#define AUDIO_BUFS     24
#define AUDIO_BUF_SIZE (44100 * AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)

struct MTY_Audio {
	MTY_Mutex *mutex;
	AudioQueueRef q;
	AudioQueueBufferRef audio_buf[AUDIO_BUFS];
	size_t queued;
	uint32_t sample_rate;
	uint32_t min_buffer;
	uint32_t max_buffer;
	bool playing;
};

static void audio_queue_buffer(MTY_Audio *ctx, AudioQueueBufferRef buf, const void *data, size_t size)
{
	if (data) {
		memcpy(buf->mAudioData, data, size);

	} else {
		memset(buf->mAudioData, 0, size);
	}

	buf->mAudioDataByteSize = size;
	buf->mUserData = "FILLED";
	ctx->queued += size;

	AudioQueueEnqueueBuffer(ctx->q, buf, 0, NULL);
}

static void audio_queue_callback(void *opaque, AudioQueueRef q, AudioQueueBufferRef buf)
{
	MTY_Audio *ctx = (MTY_Audio *) opaque;

	MTY_MutexLock(ctx->mutex);

	if (buf->mUserData) {
		ctx->queued -= buf->mAudioDataByteSize;
		buf->mUserData = NULL;
	}

	// Queue silence
	if (ctx->queued == 0)
		audio_queue_buffer(ctx, buf, NULL, AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE * (ctx->sample_rate / 10));

	MTY_MutexUnlock(ctx->mutex);
}

MTY_Audio *MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer)
{
	MTY_Audio *ctx = MTY_Alloc(1, sizeof(MTY_Audio));
	ctx->sample_rate = sampleRate;

	uint32_t frames_per_ms = lrint((float) sampleRate / 1000.0f);
	ctx->min_buffer = minBuffer * frames_per_ms;
	ctx->max_buffer = maxBuffer * frames_per_ms;

	ctx->mutex = MTY_MutexCreate();

	AudioStreamBasicDescription format = {0};
	format.mSampleRate = sampleRate;
	format.mFormatID = kAudioFormatLinearPCM;
	format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	format.mFramesPerPacket = 1;
	format.mChannelsPerFrame = AUDIO_CHANNELS;
	format.mBitsPerChannel = AUDIO_SAMPLE_SIZE * 8;
	format.mBytesPerPacket = AUDIO_SAMPLE_SIZE * AUDIO_CHANNELS;;
	format.mBytesPerFrame = format.mBytesPerPacket;

	AudioQueueNewOutput(&format, audio_queue_callback, ctx, NULL, NULL, 0, &ctx->q);

	for (int32_t x = 0; x < AUDIO_BUFS; x++)
		AudioQueueAllocateBuffer(ctx->q, AUDIO_BUF_SIZE, &ctx->audio_buf[x]);

	return ctx;
}

uint32_t MTY_AudioGetQueuedFrames(MTY_Audio *ctx)
{
	MTY_MutexLock(ctx->mutex);

	uint32_t r = ctx->queued / (AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE);

	MTY_MutexUnlock(ctx->mutex);

	return r;
}

bool MTY_AudioIsPlaying(MTY_Audio *ctx)
{
	return ctx->playing;
}

void MTY_AudioPlay(MTY_Audio *ctx)
{
	AudioQueueStart(ctx->q, NULL);
	ctx->playing = true;
}

void MTY_AudioStop(MTY_Audio *ctx)
{
	AudioQueueStop(ctx->q, true);
	ctx->playing = false;
}

void MTY_AudioQueue(MTY_Audio *ctx, const int16_t *frames, uint32_t count)
{
	size_t size = count * AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE;

	if (size <= AUDIO_BUF_SIZE) {
		MTY_MutexLock(ctx->mutex);

		for (int32_t x = 0; x < AUDIO_BUFS; x++) {
			AudioQueueBufferRef buf = ctx->audio_buf[x];

			if (!buf->mUserData) {
				audio_queue_buffer(ctx, buf, frames, size);
				break;
			}
		}

		MTY_MutexUnlock(ctx->mutex);
	}
}

void MTY_AudioDestroy(MTY_Audio **audio)
{
	if (!audio || !*audio)
		return;

	MTY_Audio *ctx = *audio;

	if (ctx->q)
		AudioQueueDispose(ctx->q, true);

	MTY_MutexDestroy(&ctx->mutex);

	MTY_Free(ctx);
	*audio = NULL;
}
