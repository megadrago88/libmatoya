// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "asound-dl.h"

#include <string.h>

#define AUDIO_BUF_SIZE (44800 * 4 * 5) // 5 seconds at 48khz

struct MTY_Audio {
	snd_pcm_t *pcm;
	bool playing;

	uint8_t *buf;
	size_t pos;
};

static void __attribute__((destructor)) audio_global_destroy(void)
{
	asound_dl_global_destroy();
}

MTY_Audio *MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer)
{
	if (!asound_dl_global_init())
		return NULL;

	MTY_Audio *ctx = MTY_Alloc(1, sizeof(MTY_Audio));

	bool r = true;

	int32_t e = snd_pcm_open(&ctx->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (e != 0) {
		MTY_Log("'snd_pcm_open' failed with error %d", e);
		r = false;
		goto except;
	}

	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(ctx->pcm, params);

	snd_pcm_hw_params_set_access(ctx->pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(ctx->pcm, params, SND_PCM_FORMAT_S16);
	snd_pcm_hw_params_set_channels(ctx->pcm, params, 2);
	snd_pcm_hw_params_set_rate(ctx->pcm, params, sampleRate, 0);
	snd_pcm_hw_params(ctx->pcm, params);
	snd_pcm_nonblock(ctx->pcm, 1);

	ctx->buf = MTY_Alloc(AUDIO_BUF_SIZE, 1);

	except:

	if (!r)
		MTY_AudioDestroy(&ctx);

	return ctx;
}

uint32_t MTY_AudioGetQueuedMs(MTY_Audio *ctx)
{
	return 0;
}

/*
uint32_t MTY_AudioGetQueuedFrames(MTY_Audio *ctx)
{
	if (ctx->playing) {
		snd_pcm_status_t *status = NULL;
		snd_pcm_status_alloca(&status);

		if (snd_pcm_status(ctx->pcm, status) >= 0) {
			snd_pcm_uframes_t avail = snd_pcm_status_get_avail(status);
			snd_pcm_uframes_t avail_max = snd_pcm_status_get_avail_max(status);

			if (avail_max > 0)
				return (int32_t) avail_max - (int32_t) avail;
		}
	} else {
		return ctx->pos / 4;
	}

	return 0;
}

bool MTY_AudioIsPlaying(MTY_Audio *ctx)
{
	return ctx->playing;
}
*/

/*
void MTY_AudioPlay(MTY_Audio *ctx)
{
	if (!ctx->playing) {
		snd_pcm_prepare(ctx->pcm);
		ctx->playing = true;
	}
}
*/

void MTY_AudioStop(MTY_Audio *ctx)
{
	ctx->playing = false;
}

void MTY_AudioQueue(MTY_Audio *ctx, const int16_t *samples, uint32_t count)
{
	size_t size = count * 4;

	if (ctx->pos + size <= AUDIO_BUF_SIZE) {
		memcpy(ctx->buf + ctx->pos, samples, count * 4);
		ctx->pos += size;
	}

	if (ctx->playing) {
		int32_t e = snd_pcm_writei(ctx->pcm, ctx->buf, ctx->pos / 4);

		if (e >= 0) {
			ctx->pos = 0;

		} else if (e == -EPIPE) {
			ctx->playing = false;
		}
	}
}

void MTY_AudioDestroy(MTY_Audio **audio)
{
	if (!audio || !*audio)
		return;

	MTY_Audio *ctx = *audio;

	if (ctx->pcm)
		snd_pcm_close(ctx->pcm);

	MTY_Free(ctx->buf);
	MTY_Free(ctx);
	*audio = NULL;
}
