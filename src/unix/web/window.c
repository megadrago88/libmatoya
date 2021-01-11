// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web.h"

struct MTY_App {
	MTY_Hash *keymap;
	MTY_Hash *hotkey;
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	void *opaque;

	MTY_GFX api;
	bool relative;
	struct gfx_ctx *gfx_ctx;
};

static void __attribute__((constructor)) app_global_init(void)
{
	web_set_mem_funcs(MTY_Alloc, MTY_Free);
}


// Keymap

static void window_hash_codes(MTY_Hash *h)
{
	MTY_HashSet(h, "Escape",          (void *) MTY_KEY_ESCAPE);
	MTY_HashSet(h, "F1",              (void *) MTY_KEY_F1);
	MTY_HashSet(h, "F2",              (void *) MTY_KEY_F2);
	MTY_HashSet(h, "F3",              (void *) MTY_KEY_F3);
	MTY_HashSet(h, "F4",              (void *) MTY_KEY_F4);
	MTY_HashSet(h, "F5",              (void *) MTY_KEY_F5);
	MTY_HashSet(h, "F6",              (void *) MTY_KEY_F6);
	MTY_HashSet(h, "F7",              (void *) MTY_KEY_F7);
	MTY_HashSet(h, "F8",              (void *) MTY_KEY_F8);
	MTY_HashSet(h, "F9",              (void *) MTY_KEY_F9);
	MTY_HashSet(h, "F10",             (void *) MTY_KEY_F10);
	MTY_HashSet(h, "F11",             (void *) MTY_KEY_F11);
	MTY_HashSet(h, "F12",             (void *) MTY_KEY_F12);

	MTY_HashSet(h, "Backquote",       (void *) MTY_KEY_GRAVE);
	MTY_HashSet(h, "Digit1",          (void *) MTY_KEY_1);
	MTY_HashSet(h, "Digit2",          (void *) MTY_KEY_2);
	MTY_HashSet(h, "Digit3",          (void *) MTY_KEY_3);
	MTY_HashSet(h, "Digit4",          (void *) MTY_KEY_4);
	MTY_HashSet(h, "Digit5",          (void *) MTY_KEY_5);
	MTY_HashSet(h, "Digit6",          (void *) MTY_KEY_6);
	MTY_HashSet(h, "Digit7",          (void *) MTY_KEY_7);
	MTY_HashSet(h, "Digit8",          (void *) MTY_KEY_8);
	MTY_HashSet(h, "Digit9",          (void *) MTY_KEY_9);
	MTY_HashSet(h, "Digit0",          (void *) MTY_KEY_0);
	MTY_HashSet(h, "Minus",           (void *) MTY_KEY_MINUS);
	MTY_HashSet(h, "Equal",           (void *) MTY_KEY_EQUALS);
	MTY_HashSet(h, "Backspace",       (void *) MTY_KEY_BACKSPACE);

	MTY_HashSet(h, "Tab",             (void *) MTY_KEY_TAB);
	MTY_HashSet(h, "KeyQ",            (void *) MTY_KEY_Q);
	MTY_HashSet(h, "KeyW",            (void *) MTY_KEY_W);
	MTY_HashSet(h, "KeyE",            (void *) MTY_KEY_E);
	MTY_HashSet(h, "KeyR",            (void *) MTY_KEY_R);
	MTY_HashSet(h, "KeyT",            (void *) MTY_KEY_T);
	MTY_HashSet(h, "KeyY",            (void *) MTY_KEY_Y);
	MTY_HashSet(h, "KeyU",            (void *) MTY_KEY_U);
	MTY_HashSet(h, "KeyI",            (void *) MTY_KEY_I);
	MTY_HashSet(h, "KeyO",            (void *) MTY_KEY_O);
	MTY_HashSet(h, "KeyP",            (void *) MTY_KEY_P);
	MTY_HashSet(h, "BracketLeft",     (void *) MTY_KEY_LBRACKET);
	MTY_HashSet(h, "BracketRight",    (void *) MTY_KEY_RBRACKET);
	MTY_HashSet(h, "Backslash",       (void *) MTY_KEY_BACKSLASH);

	MTY_HashSet(h, "CapsLock",        (void *) MTY_KEY_CAPS);
	MTY_HashSet(h, "KeyA",            (void *) MTY_KEY_A);
	MTY_HashSet(h, "KeyS",            (void *) MTY_KEY_S);
	MTY_HashSet(h, "KeyD",            (void *) MTY_KEY_D);
	MTY_HashSet(h, "KeyF",            (void *) MTY_KEY_F);
	MTY_HashSet(h, "KeyG",            (void *) MTY_KEY_G);
	MTY_HashSet(h, "KeyH",            (void *) MTY_KEY_H);
	MTY_HashSet(h, "KeyJ",            (void *) MTY_KEY_J);
	MTY_HashSet(h, "KeyK",            (void *) MTY_KEY_K);
	MTY_HashSet(h, "KeyL",            (void *) MTY_KEY_L);
	MTY_HashSet(h, "Semicolon",       (void *) MTY_KEY_SEMICOLON);
	MTY_HashSet(h, "Quote",           (void *) MTY_KEY_QUOTE);
	MTY_HashSet(h, "Enter",           (void *) MTY_KEY_ENTER);

	MTY_HashSet(h, "ShiftLeft",       (void *) MTY_KEY_LSHIFT);
	MTY_HashSet(h, "KeyZ",            (void *) MTY_KEY_Z);
	MTY_HashSet(h, "KeyX",            (void *) MTY_KEY_X);
	MTY_HashSet(h, "KeyC",            (void *) MTY_KEY_C);
	MTY_HashSet(h, "KeyV",            (void *) MTY_KEY_V);
	MTY_HashSet(h, "KeyB",            (void *) MTY_KEY_B);
	MTY_HashSet(h, "KeyN",            (void *) MTY_KEY_N);
	MTY_HashSet(h, "KeyM",            (void *) MTY_KEY_M);
	MTY_HashSet(h, "Comma",           (void *) MTY_KEY_COMMA);
	MTY_HashSet(h, "Period",          (void *) MTY_KEY_PERIOD);
	MTY_HashSet(h, "ShiftRight",      (void *) MTY_KEY_RSHIFT);

	MTY_HashSet(h, "ControlLeft",     (void *) MTY_KEY_LCTRL);
	MTY_HashSet(h, "MetaLeft",        (void *) MTY_KEY_LWIN);
	MTY_HashSet(h, "AltLeft",         (void *) MTY_KEY_LALT);
	MTY_HashSet(h, "Space",           (void *) MTY_KEY_SPACE);
	MTY_HashSet(h, "AltRight",        (void *) MTY_KEY_RALT);
	MTY_HashSet(h, "MetaRight",       (void *) MTY_KEY_RWIN);
	MTY_HashSet(h, "ContextMenu",     (void *) MTY_KEY_APP);
	MTY_HashSet(h, "ControlRight",    (void *) MTY_KEY_RCTRL);

	MTY_HashSet(h, "PrintScreen",     (void *) MTY_KEY_PRINT_SCREEN);
	MTY_HashSet(h, "ScrollLock",      (void *) MTY_KEY_SCROLL_LOCK);
	MTY_HashSet(h, "Pause",           (void *) MTY_KEY_PAUSE);

	MTY_HashSet(h, "Insert",          (void *) MTY_KEY_INSERT);
	MTY_HashSet(h, "Home",            (void *) MTY_KEY_HOME);
	MTY_HashSet(h, "PageUp",          (void *) MTY_KEY_PAGE_UP);
	MTY_HashSet(h, "Delete",          (void *) MTY_KEY_DELETE);
	MTY_HashSet(h, "End",             (void *) MTY_KEY_END);
	MTY_HashSet(h, "PageDown",        (void *) MTY_KEY_PAGE_DOWN);

	MTY_HashSet(h, "ArrowUp",         (void *) MTY_KEY_UP);
	MTY_HashSet(h, "ArrowLeft",       (void *) MTY_KEY_LEFT);
	MTY_HashSet(h, "ArrowDown",       (void *) MTY_KEY_DOWN);
	MTY_HashSet(h, "ArrowRight",      (void *) MTY_KEY_RIGHT);

	MTY_HashSet(h, "NumLock",         (void *) MTY_KEY_NUM_LOCK);
	MTY_HashSet(h, "NumpadDivide",    (void *) MTY_KEY_NP_DIVIDE);
	MTY_HashSet(h, "NumpadMultiply",  (void *) MTY_KEY_NP_MULTIPLY);
	MTY_HashSet(h, "NumpadSubtract",  (void *) MTY_KEY_NP_MINUS);
	MTY_HashSet(h, "Numpad7",         (void *) MTY_KEY_NP_7);
	MTY_HashSet(h, "Numpad8",         (void *) MTY_KEY_NP_8);
	MTY_HashSet(h, "Numpad9",         (void *) MTY_KEY_NP_9);
	MTY_HashSet(h, "NumpadAdd",       (void *) MTY_KEY_NP_PLUS);
	MTY_HashSet(h, "Numpad4",         (void *) MTY_KEY_NP_4);
	MTY_HashSet(h, "Numpad5",         (void *) MTY_KEY_NP_5);
	MTY_HashSet(h, "Numpad6",         (void *) MTY_KEY_NP_6);
	MTY_HashSet(h, "Numpad1",         (void *) MTY_KEY_NP_1);
	MTY_HashSet(h, "Numpad2",         (void *) MTY_KEY_NP_2);
	MTY_HashSet(h, "Numpad3",         (void *) MTY_KEY_NP_3);
	MTY_HashSet(h, "NumpadEnter",     (void *) MTY_KEY_NP_ENTER);
	MTY_HashSet(h, "Numpad0",         (void *) MTY_KEY_NP_0);
	MTY_HashSet(h, "NumpadDecimal",   (void *) MTY_KEY_NP_PERIOD);
}


// Events

static void window_mouse_motion(MTY_App *ctx, bool relative, int32_t x, int32_t y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.relative = relative;
	msg.mouseMotion.x = x;
	msg.mouseMotion.y = y;

	ctx->msg_func(&msg, ctx->opaque);
}

static void window_mouse_button(MTY_App *ctx, bool pressed, int32_t button, int32_t x, int32_t y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = pressed;
	msg.mouseButton.button =
		button == 0 ? MTY_MOUSE_BUTTON_LEFT :
		button == 1 ? MTY_MOUSE_BUTTON_MIDDLE :
		button == 2 ? MTY_MOUSE_BUTTON_RIGHT :
		button == 3 ? MTY_MOUSE_BUTTON_X1 :
		button == 4 ? MTY_MOUSE_BUTTON_X2 :
		MTY_MOUSE_BUTTON_NONE;

	// Simulate movement to where the click occurs
	if (pressed && !ctx->relative) {
		MTY_Msg mmsg = {0};
		mmsg.type = MTY_MSG_MOUSE_MOTION;
		mmsg.mouseMotion.relative = false;
		mmsg.mouseMotion.click = true;
		mmsg.mouseMotion.x = x;
		mmsg.mouseMotion.y = y;

		ctx->msg_func(&mmsg, ctx->opaque);
	}

	ctx->msg_func(&msg, ctx->opaque);
}

static void window_mouse_wheel(MTY_App *ctx, int32_t x, int32_t y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.x = x;
	msg.mouseWheel.y = -y;

	ctx->msg_func(&msg, ctx->opaque);
}

static void app_kb_to_hotkey(MTY_App *app, MTY_Msg *msg)
{
	MTY_Mod mod = msg->keyboard.mod & 0xFF;
	uint32_t hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(app->hotkey, (mod << 16) | msg->keyboard.key);

	if (hotkey != 0) {
		if (msg->keyboard.pressed) {
			msg->type = MTY_MSG_HOTKEY;
			msg->hotkey = hotkey;

		} else {
			msg->type = MTY_MSG_NONE;
		}
	}
}

static void window_keyboard(MTY_App *ctx, bool pressed, uint32_t keyCode, const char *code,
	const char *key, uint32_t mods)
{
	MTY_Msg msg = {0};

	if (key) {
		msg.type = MTY_MSG_TEXT;
		snprintf(msg.text, 8, "%s", key);

		ctx->msg_func(&msg, ctx->opaque);
	}

	msg.keyboard.key = (MTY_Key) MTY_HashGet(ctx->keymap, code);

	if (msg.keyboard.key != 0) {
		msg.type = MTY_MSG_KEYBOARD;
		msg.keyboard.pressed = pressed;

		if (mods & 0x01) msg.keyboard.mod |= MTY_MOD_LSHIFT;
		if (mods & 0x02) msg.keyboard.mod |= MTY_MOD_LCTRL;
		if (mods & 0x04) msg.keyboard.mod |= MTY_MOD_LALT;
		if (mods & 0x08) msg.keyboard.mod |= MTY_MOD_LWIN;
		if (mods & 0x10) msg.keyboard.mod |= MTY_MOD_CAPS;
		if (mods & 0x20) msg.keyboard.mod |= MTY_MOD_NUM;

		app_kb_to_hotkey(ctx, &msg);
		ctx->msg_func(&msg, ctx->opaque);
	}
}

static void window_focus(MTY_App *ctx, bool focus)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_FOCUS;
	msg.focus = focus;

	ctx->msg_func(&msg, ctx->opaque);
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


// App / Window

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	MTY_Strcat(str, len, (mod & MTY_MOD_WIN) ? "Super+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_SHIFT) ? "Shift+" : "");

	// TODO
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	mod &= 0xFF;
	MTY_HashSetInt(ctx->hotkey, (mod << 16) | key, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
	MTY_HashDestroy(&ctx->hotkey, NULL);
	ctx->hotkey = MTY_HashCreate(0);
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return web_get_clipboard_text();
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	web_set_clipboard_text(text);
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	// TODO
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	// TODO
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	// TODO
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	web_create_canvas();
	web_attach_events(ctx, window_mouse_motion, window_mouse_button,
		window_mouse_wheel, window_keyboard, window_focus, window_drop);

	ctx->hotkey = MTY_HashCreate(0);
	ctx->keymap = MTY_HashCreate(0);

	window_hash_codes(ctx->keymap);

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_HashDestroy(&ctx->hotkey, NULL);
	MTY_HashDestroy(&ctx->keymap, NULL);

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *ctx)
{
	web_raf(ctx->app_func, ctx->opaque);
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	// TODO
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	// TODO

	return MTY_DETACH_NONE;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	// TODO
	// const wl = await navigator.wakeLock.request("screen");
	// wl.release();
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	// TODO
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	web_set_pointer_lock(relative);
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return web_get_pointer_lock();
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return web_has_focus();
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	// TODO
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
	web_get_screen_size(width, height);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return web_get_pixel_ratio();
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return web_is_visible();
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return web_has_focus();
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

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return false;
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
