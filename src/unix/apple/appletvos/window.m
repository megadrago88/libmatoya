// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

struct MTY_Window {
	bool dummy;
};

MTY_Window *MTY_WindowCreate(const char *title, MTY_MsgFunc msg_func, const void *opaque,
	uint32_t width, uint32_t height, bool fullscreen)
{
	return NULL;
}

void MTY_AppRun(MTY_AppFunc func, const void *opaque)
{
}

void MTY_WindowSetTitle(MTY_Window *ctx, const char *title, const char *subtitle)
{
}

bool MTY_WindowGetSize(MTY_Window *ctx, uint32_t *width, uint32_t *height)
{
	return true;
}

void MTY_WindowPoll(MTY_Window *ctx)
{
}

bool MTY_WindowIsForeground(MTY_Window *ctx)
{
	return true;
}

uint32_t MTY_WindowGetRefreshRate(MTY_Window *ctx)
{
	return 60;
}

float MTY_WindowGetDPIScale(MTY_Window *ctx)
{
	return 1.0f;
}

void MTY_WindowSetFullscreen(MTY_Window *ctx)
{
}

void MTY_WindowSetSize(MTY_Window *ctx, uint32_t width, uint32_t height)
{
}

bool MTY_WindowIsFullscreen(MTY_Window *ctx)
{
	return false;
}

void MTY_WindowPresent(MTY_Window *ctx, uint32_t num_frames)
{
}

MTY_Device *MTY_WindowGetDevice(MTY_Window *ctx)
{
	return NULL;
}

MTY_Context *MTY_WindowGetContext(MTY_Window *ctx)
{
	return NULL;
}

MTY_Texture *MTY_WindowGetBackBuffer(MTY_Window *ctx)
{
	return NULL;
}

void MTY_WindowDrawQuad(MTY_Window *ctx, const void *image, const MTY_RenderDesc *desc)
{
}

void MTY_WindowDrawUI(MTY_Window *ctx, const MTY_DrawData *dd)
{
}

void MTY_WindowSetUIFont(MTY_Window *ctx, const void *font, uint32_t width, uint32_t height)
{
}

void *MTY_WindowGetUIFontResource(MTY_Window *ctx)
{
	return NULL;
}

void MTY_WindowDestroy(MTY_Window **window)
{
}
