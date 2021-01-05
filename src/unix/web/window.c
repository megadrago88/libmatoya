// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>

#include "web.h"

struct MTY_App {
	MTY_Hash *h;
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	void *opaque;

	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
};

static void window_hash_codes(MTY_Hash *h)
{
	MTY_HashSet(h, "Key1", (void *) MTY_KEY_1);
	MTY_HashSet(h, "KeyA", (void *) MTY_KEY_A);
	MTY_HashSet(h, "KeyC", (void *) MTY_KEY_C);
	MTY_HashSet(h, "KeyD", (void *) MTY_KEY_D);
	MTY_HashSet(h, "KeyL", (void *) MTY_KEY_L);
	MTY_HashSet(h, "KeyM", (void *) MTY_KEY_M);
	MTY_HashSet(h, "KeyO", (void *) MTY_KEY_O);
	MTY_HashSet(h, "KeyP", (void *) MTY_KEY_P);
	MTY_HashSet(h, "KeyR", (void *) MTY_KEY_R);
	MTY_HashSet(h, "KeyS", (void *) MTY_KEY_S);
	MTY_HashSet(h, "KeyT", (void *) MTY_KEY_T);
	MTY_HashSet(h, "KeyV", (void *) MTY_KEY_V);
	MTY_HashSet(h, "KeyW", (void *) MTY_KEY_W);
	MTY_HashSet(h, "KeyX", (void *) MTY_KEY_X);
	MTY_HashSet(h, "KeyY", (void *) MTY_KEY_Y);
	MTY_HashSet(h, "KeyZ", (void *) MTY_KEY_Z);
	MTY_HashSet(h, "Space", (void *) MTY_KEY_SPACE);
	MTY_HashSet(h, "ArrowDown", (void *) MTY_KEY_DOWN);
	MTY_HashSet(h, "ArrowLeft", (void *) MTY_KEY_LEFT);
	MTY_HashSet(h, "ArrowUp", (void *) MTY_KEY_UP);
	MTY_HashSet(h, "ArrowRight", (void *) MTY_KEY_RIGHT);
	MTY_HashSet(h, "Escape", (void *) MTY_KEY_ESCAPE);
	MTY_HashSet(h, "Semicolon", (void *) MTY_KEY_SEMICOLON);
	MTY_HashSet(h, "ShiftLeft", (void *) MTY_KEY_LSHIFT);
	MTY_HashSet(h, "ShiftRight", (void *) MTY_KEY_RSHIFT);
	MTY_HashSet(h, "ControlLeft", (void *) MTY_KEY_LCTRL);
	MTY_HashSet(h, "ControlRight", (void *) MTY_KEY_RCTRL);
	MTY_HashSet(h, "AltLeft", (void *) MTY_KEY_LALT);
	MTY_HashSet(h, "AltRight", (void *) MTY_KEY_RALT);
}

static void window_mouse_motion(MTY_App *ctx, int32_t x, int32_t y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = x;
	msg.mouseMotion.y = y;

	ctx->msg_func(&msg, ctx->opaque);
}

static void window_mouse_button(MTY_App *ctx, bool pressed, int32_t button)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = pressed;
	msg.mouseButton.button = button == 1 ? MTY_MOUSE_BUTTON_LEFT :
		button == 2 ? MTY_MOUSE_BUTTON_MIDDLE : button == 3 ? MTY_MOUSE_BUTTON_RIGHT : 0;

	ctx->msg_func(&msg, ctx->opaque);
}

static void window_mouse_wheel(MTY_App *ctx, int32_t x, int32_t y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.x = x;
	msg.mouseWheel.y = y;

	ctx->msg_func(&msg, ctx->opaque);
}

static void window_keyboard(MTY_App *ctx, bool pressed, const char *code)
{
	MTY_Msg msg = {0};
	msg.keyboard.key = (MTY_Key) MTY_HashGet(ctx->h, code);

	if (msg.keyboard.key != 0) {
		msg.type = MTY_MSG_KEYBOARD;
		msg.keyboard.pressed = pressed;

		ctx->msg_func(&msg, ctx->opaque);
	}
}

static void window_drop(MTY_App *ctx, const char *name, const void *data, size_t size)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_DROP;
	msg.drop.name = name;
	msg.drop.data = data;
	msg.drop.size = size;

	ctx->msg_func(&msg, ctx->opaque);
}

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	web_create_canvas();
	web_attach_events(ctx, malloc, free, window_mouse_motion,
		window_mouse_button, window_mouse_wheel, window_keyboard, window_drop);

	ctx->h = MTY_HashCreate(100);
	window_hash_codes(ctx->h);

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
	web_raf(ctx->app_func, ctx->opaque);
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return MTY_DETACH_NONE;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return false;
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return true;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	MTY_WindowSetTitle(app, 0, title);

	return 0;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	web_set_title(title);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	web_get_size(width, height);

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	*width = *height = 600;

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return web_get_pixel_ratio();
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return false;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return true;
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return true;
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


// Unimplemented

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
