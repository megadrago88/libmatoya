// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>

#include <jni.h>
#include <android/log.h>

struct MTY_App {
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Detach detach;
	void *opaque;

	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
};


// JNI

int main(int argc, char **argv);

static uint32_t APP_WIDTH = 1920;
static uint32_t APP_HEIGHT = 1080;

JNIEXPORT void JNICALL Java_group_matoya_lib_MainSurface_mty_1window_1dims(JNIEnv *env, jobject instance, jint w, jint h)
{
	APP_WIDTH = w;
	APP_HEIGHT = h;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_AppThread_mty_1start(JNIEnv *env, jobject instance, jstring jname)
{
	const char *cname = (*env)->GetStringUTFChars(env, jname, 0);
	char *name = MTY_Strdup(cname);

	(*env)->ReleaseStringUTFChars(env, jname, cname);

	// __android_log_print(ANDROID_LOG_INFO, "MTY", "START");

	main(1, &name);

	MTY_Free(name);
}


// App / window

char *MTY_AppGetClipboard(MTY_App *app)
{
	// TODO
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	// TODO
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *ctx)
{
	// TODO
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	// TODO
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	// TODO
	return true;
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	// TODO
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	*width = APP_WIDTH;
	*height = APP_HEIGHT;

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	// TODO
	return 1.0f;
}


// Window Private

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	app->api = api;
	app->gfx_ctx = gfx_ctx;
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	if (gfx_ctx)
		*gfx_ctx = app->gfx_ctx;

	return app->api;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	return (void *) (uintptr_t) 0xCDD;
}


// Unimplemented / stubs

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return false;
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return MTY_AppIsActive(app);
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return MTY_AppIsActive(app);
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return 0;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return MTY_WindowGetScreenSize(app, window, width, height);
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return false;
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout)
{
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

void *MTY_GLGetProcAddress(const char *name)
{
	return NULL;
}

void MTY_AppSetTray(MTY_App *app, const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
}

void MTY_AppRemoveTray(MTY_App *app)
{
}

void MTY_AppNotification(MTY_App *app, const char *title, const char *msg)
{
}

void MTY_AppSetOnscreenKeyboard(MTY_App *app, bool enable)
{
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}
