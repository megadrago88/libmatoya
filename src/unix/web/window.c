// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "web.h"

static MTY_Hash *KEYREV;

struct MTY_App {
	MTY_Hash *keymap;
	MTY_Hash *hotkey;
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Detach detach;
	void *opaque;

	MTY_GFX api;
	bool kb_grab;
	struct gfx_ctx *gfx_ctx;
};

static void __attribute__((constructor)) app_global_init(void)
{
	web_set_mem_funcs(MTY_Alloc, MTY_Free);
}


// Keymap

static void window_map_code(MTY_Hash *h, MTY_Hash *rev, const char *code, MTY_Key key)
{
	MTY_HashSet(h, code, (void *) key);

	char *buf = MTY_Alloc(32, 1);

	if (!web_get_key(code, buf, 32))
		snprintf(buf, 32, "%s", code);

	MTY_HashSetInt(rev, key, buf);
}

static void window_hash_codes(MTY_Hash *h, MTY_Hash *rev)
{
	// TODO JIS/ISO keys

	window_map_code(h, rev, "Escape",          MTY_KEY_ESCAPE);
	window_map_code(h, rev, "F1",              MTY_KEY_F1);
	window_map_code(h, rev, "F2",              MTY_KEY_F2);
	window_map_code(h, rev, "F3",              MTY_KEY_F3);
	window_map_code(h, rev, "F4",              MTY_KEY_F4);
	window_map_code(h, rev, "F5",              MTY_KEY_F5);
	window_map_code(h, rev, "F6",              MTY_KEY_F6);
	window_map_code(h, rev, "F7",              MTY_KEY_F7);
	window_map_code(h, rev, "F8",              MTY_KEY_F8);
	window_map_code(h, rev, "F9",              MTY_KEY_F9);
	window_map_code(h, rev, "F10",             MTY_KEY_F10);
	window_map_code(h, rev, "F11",             MTY_KEY_F11);
	window_map_code(h, rev, "F12",             MTY_KEY_F12);

	window_map_code(h, rev, "Backquote",       MTY_KEY_GRAVE);
	window_map_code(h, rev, "Digit1",          MTY_KEY_1);
	window_map_code(h, rev, "Digit2",          MTY_KEY_2);
	window_map_code(h, rev, "Digit3",          MTY_KEY_3);
	window_map_code(h, rev, "Digit4",          MTY_KEY_4);
	window_map_code(h, rev, "Digit5",          MTY_KEY_5);
	window_map_code(h, rev, "Digit6",          MTY_KEY_6);
	window_map_code(h, rev, "Digit7",          MTY_KEY_7);
	window_map_code(h, rev, "Digit8",          MTY_KEY_8);
	window_map_code(h, rev, "Digit9",          MTY_KEY_9);
	window_map_code(h, rev, "Digit0",          MTY_KEY_0);
	window_map_code(h, rev, "Minus",           MTY_KEY_MINUS);
	window_map_code(h, rev, "Equal",           MTY_KEY_EQUALS);
	window_map_code(h, rev, "Backspace",       MTY_KEY_BACKSPACE);

	window_map_code(h, rev, "Tab",             MTY_KEY_TAB);
	window_map_code(h, rev, "KeyQ",            MTY_KEY_Q);
	window_map_code(h, rev, "KeyW",            MTY_KEY_W);
	window_map_code(h, rev, "KeyE",            MTY_KEY_E);
	window_map_code(h, rev, "KeyR",            MTY_KEY_R);
	window_map_code(h, rev, "KeyT",            MTY_KEY_T);
	window_map_code(h, rev, "KeyY",            MTY_KEY_Y);
	window_map_code(h, rev, "KeyU",            MTY_KEY_U);
	window_map_code(h, rev, "KeyI",            MTY_KEY_I);
	window_map_code(h, rev, "KeyO",            MTY_KEY_O);
	window_map_code(h, rev, "KeyP",            MTY_KEY_P);
	window_map_code(h, rev, "BracketLeft",     MTY_KEY_LBRACKET);
	window_map_code(h, rev, "BracketRight",    MTY_KEY_RBRACKET);
	window_map_code(h, rev, "Backslash",       MTY_KEY_BACKSLASH);

	window_map_code(h, rev, "CapsLock",        MTY_KEY_CAPS);
	window_map_code(h, rev, "KeyA",            MTY_KEY_A);
	window_map_code(h, rev, "KeyS",            MTY_KEY_S);
	window_map_code(h, rev, "KeyD",            MTY_KEY_D);
	window_map_code(h, rev, "KeyF",            MTY_KEY_F);
	window_map_code(h, rev, "KeyG",            MTY_KEY_G);
	window_map_code(h, rev, "KeyH",            MTY_KEY_H);
	window_map_code(h, rev, "KeyJ",            MTY_KEY_J);
	window_map_code(h, rev, "KeyK",            MTY_KEY_K);
	window_map_code(h, rev, "KeyL",            MTY_KEY_L);
	window_map_code(h, rev, "Semicolon",       MTY_KEY_SEMICOLON);
	window_map_code(h, rev, "Quote",           MTY_KEY_QUOTE);
	window_map_code(h, rev, "Enter",           MTY_KEY_ENTER);

	window_map_code(h, rev, "ShiftLeft",       MTY_KEY_LSHIFT);
	window_map_code(h, rev, "KeyZ",            MTY_KEY_Z);
	window_map_code(h, rev, "KeyX",            MTY_KEY_X);
	window_map_code(h, rev, "KeyC",            MTY_KEY_C);
	window_map_code(h, rev, "KeyV",            MTY_KEY_V);
	window_map_code(h, rev, "KeyB",            MTY_KEY_B);
	window_map_code(h, rev, "KeyN",            MTY_KEY_N);
	window_map_code(h, rev, "KeyM",            MTY_KEY_M);
	window_map_code(h, rev, "Comma",           MTY_KEY_COMMA);
	window_map_code(h, rev, "Period",          MTY_KEY_PERIOD);
	window_map_code(h, rev, "ShiftRight",      MTY_KEY_RSHIFT);

	window_map_code(h, rev, "ControlLeft",     MTY_KEY_LCTRL);
	window_map_code(h, rev, "MetaLeft",        MTY_KEY_LWIN);
	window_map_code(h, rev, "AltLeft",         MTY_KEY_LALT);
	window_map_code(h, rev, "Space",           MTY_KEY_SPACE);
	window_map_code(h, rev, "AltRight",        MTY_KEY_RALT);
	window_map_code(h, rev, "MetaRight",       MTY_KEY_RWIN);
	window_map_code(h, rev, "ContextMenu",     MTY_KEY_APP);
	window_map_code(h, rev, "ControlRight",    MTY_KEY_RCTRL);

	window_map_code(h, rev, "PrintScreen",     MTY_KEY_PRINT_SCREEN);
	window_map_code(h, rev, "ScrollLock",      MTY_KEY_SCROLL_LOCK);
	window_map_code(h, rev, "Pause",           MTY_KEY_PAUSE);

	window_map_code(h, rev, "Insert",          MTY_KEY_INSERT);
	window_map_code(h, rev, "Home",            MTY_KEY_HOME);
	window_map_code(h, rev, "PageUp",          MTY_KEY_PAGE_UP);
	window_map_code(h, rev, "Delete",          MTY_KEY_DELETE);
	window_map_code(h, rev, "End",             MTY_KEY_END);
	window_map_code(h, rev, "PageDown",        MTY_KEY_PAGE_DOWN);

	window_map_code(h, rev, "ArrowUp",         MTY_KEY_UP);
	window_map_code(h, rev, "ArrowLeft",       MTY_KEY_LEFT);
	window_map_code(h, rev, "ArrowDown",       MTY_KEY_DOWN);
	window_map_code(h, rev, "ArrowRight",      MTY_KEY_RIGHT);

	window_map_code(h, rev, "NumLock",         MTY_KEY_NUM_LOCK);
	window_map_code(h, rev, "NumpadDivide",    MTY_KEY_NP_DIVIDE);
	window_map_code(h, rev, "NumpadMultiply",  MTY_KEY_NP_MULTIPLY);
	window_map_code(h, rev, "NumpadSubtract",  MTY_KEY_NP_MINUS);
	window_map_code(h, rev, "Numpad7",         MTY_KEY_NP_7);
	window_map_code(h, rev, "Numpad8",         MTY_KEY_NP_8);
	window_map_code(h, rev, "Numpad9",         MTY_KEY_NP_9);
	window_map_code(h, rev, "NumpadAdd",       MTY_KEY_NP_PLUS);
	window_map_code(h, rev, "Numpad4",         MTY_KEY_NP_4);
	window_map_code(h, rev, "Numpad5",         MTY_KEY_NP_5);
	window_map_code(h, rev, "Numpad6",         MTY_KEY_NP_6);
	window_map_code(h, rev, "Numpad1",         MTY_KEY_NP_1);
	window_map_code(h, rev, "Numpad2",         MTY_KEY_NP_2);
	window_map_code(h, rev, "Numpad3",         MTY_KEY_NP_3);
	window_map_code(h, rev, "NumpadEnter",     MTY_KEY_NP_ENTER);
	window_map_code(h, rev, "Numpad0",         MTY_KEY_NP_0);
	window_map_code(h, rev, "NumpadDecimal",   MTY_KEY_NP_PERIOD);
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
	if (pressed && !web_get_pointer_lock()) {
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

static bool window_allow_default(MTY_Mod mod, MTY_Key key)
{
	// The "allowed" browser hotkey list. Refresh, fullscreen, developer console, and tab switching

	return
		((mod & MTY_MOD_CTRL) && (mod & MTY_MOD_SHIFT) && key == MTY_KEY_I) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_R) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_F5) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_1) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_2) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_3) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_4) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_5) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_6) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_7) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_8) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_9) ||
		(key == MTY_KEY_F5) ||
		(key == MTY_KEY_F11) ||
		(key == MTY_KEY_F12);
}

static bool window_keyboard(MTY_App *ctx, bool pressed, uint32_t keyCode, const char *code,
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

		return !window_allow_default(msg.keyboard.mod, msg.keyboard.key) || ctx->kb_grab;
	}

	return true;
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

static void window_controller(MTY_App *ctx, uint32_t id, uint32_t state, uint32_t buttons,
	float lx, float ly, float rx, float ry, float lt, float rt)
{
	#define TEST_BUTTON(i) \
		((buttons & (i)) == (i))

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_CONTROLLER;

	MTY_Controller *c = &msg.controller;
	c->driver = MTY_HID_DRIVER_DEFAULT;
	c->numButtons = 16;
	c->numValues = 7;
	c->vid = 0xCDD;
	c->pid = 0xCDD;
	c->id = id;

	c->buttons[MTY_CBUTTON_A]              = TEST_BUTTON(0x0001);
	c->buttons[MTY_CBUTTON_B]              = TEST_BUTTON(0x0002);
	c->buttons[MTY_CBUTTON_X]              = TEST_BUTTON(0x0004);
	c->buttons[MTY_CBUTTON_Y]              = TEST_BUTTON(0x0008);
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER]  = TEST_BUTTON(0x0010);
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = TEST_BUTTON(0x0020);
	c->buttons[MTY_CBUTTON_BACK]           = TEST_BUTTON(0x0100);
	c->buttons[MTY_CBUTTON_START]          = TEST_BUTTON(0x0200);
	c->buttons[MTY_CBUTTON_LEFT_THUMB]     = TEST_BUTTON(0x0400);
	c->buttons[MTY_CBUTTON_RIGHT_THUMB]    = TEST_BUTTON(0x0800);

	c->values[MTY_CVALUE_THUMB_LX].data = lx < 0.0f ? lrint(lx * abs(INT16_MIN)) : lrint(lx * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_LY].data = ly > 0.0f ? lrint(-ly * abs(INT16_MIN)) : lrint(-ly * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = rx < 0.0f ? lrint(rx * abs(INT16_MIN)) : lrint(rx * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RY].data = ry > 0.0f ? lrint(-ry * abs(INT16_MIN)) : lrint(-ry * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = lrint(lt * UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = lrint(rt * UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_L].data > 0;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_R].data > 0;

	bool up = TEST_BUTTON(0x1000);
	bool down = TEST_BUTTON(0x2000);
	bool left = TEST_BUTTON(0x4000);
	bool right = TEST_BUTTON(0x8000);

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;

	// Connect
	if (state == 1) {
		MTY_Msg cmsg = msg;
		cmsg.type = MTY_MSG_CONNECT;
		ctx->msg_func(&cmsg, ctx->opaque);

	// Disconnect
	} else if (state == 2) {
		msg.type = MTY_MSG_DISCONNECT;
	}

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

	const char *key_str = MTY_HashGetInt(KEYREV, key);
	if (key_str)
		MTY_Strcat(str, len, key_str);
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
	web_set_png_cursor(image, size, hotX, hotY);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	web_use_default_cursor(useDefault);
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	web_show_cursor(show);
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
	KEYREV = MTY_HashCreate(0);

	window_hash_codes(ctx->keymap, KEYREV);

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_HashDestroy(&ctx->hotkey, NULL);
	MTY_HashDestroy(&ctx->keymap, NULL);
	MTY_HashDestroy(&KEYREV, MTY_Free);

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *ctx)
{
	web_raf(ctx, ctx->app_func, window_controller, ctx->opaque);
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	// TODO
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	// TODO
	// const wl = await navigator.wakeLock.request("screen");
	// wl.release();
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	app->kb_grab = grab;
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
	web_rumble_gamepad(id, (float) low / (float) UINT16_MAX, (float) high / (float) UINT16_MAX);
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
