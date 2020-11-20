// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "x-dl.h"

struct MTY_Window {
	Window window;
	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
	Atom wm_delete_window;
};

struct MTY_App {
	Display *display;
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	void *opaque;
	float dpi;
};

#define WINDOW_MASK (   \
	ExposureMask      | \
	KeyPressMask      | \
	KeyReleaseMask    | \
	ButtonPressMask   | \
	ButtonReleaseMask | \
	PointerMotionMask   \
)

static void __attribute__((destructor)) app_global_destroy(void)
{
	x_dl_global_destroy();
}

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
{
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Hotkey mode, MTY_Keymod mod, MTY_Scancode scancode, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Hotkey mode)
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
	if (!x_dl_global_init())
		return NULL;

	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));

	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display) {r = false; goto except;}

	GLint attr[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};

	XVisualInfo *vis = glXChooseVisual(ctx->display, 0, attr);
	if (!vis) {r = false; goto except;}

	const char *dpi_s = XGetDefault(ctx->display, "Xft", "dpi");
	ctx->dpi = dpi_s ? (float) atoi(dpi_s) / (float) 96.0f : 1.0f;

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	if (ctx->display)
		XCloseDisplay(ctx->display);

	MTY_Free(*app);
	*app = NULL;
}

static void app_events(MTY_App *app)
{
	while (XPending(ctx->display) > 0) {
		MTY_Msg wmsg = {0};

		XEvent event = {0};
		XNextEvent(ctx->display, &event);

		switch (event.type) {
			case KeyPress:
			case KeyRelease: {
				wmsg.type = MTY_WINDOW_MSG_KEYBOARD;
				wmsg.keyboard.pressed = event.type == KeyPress;
				wmsg.keyboard.scancode = window_x_to_mty(XLookupKeysym(&event.xkey, 0));
				break;
			}
			case ButtonPress:
			case ButtonRelease:
				wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
				wmsg.mouseButton.pressed = event.type == ButtonPress;

				switch (event.xbutton.button) {
					case 1: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_L;      break;
					case 2: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_MIDDLE; break;
					case 3: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_R;      break;

					// Mouse wheel
					case 4:
					case 5:
						wmsg.type = MTY_WINDOW_MSG_MOUSE_WHEEL;
						wmsg.mouseWheel.x = 0;
						wmsg.mouseWheel.y = event.xbutton.button == 4 ? -100 : 100;
						break;
				}
				break;
			case MotionNotify:
				wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
				wmsg.mouseMotion.relative = false;
				wmsg.mouseMotion.x = event.xmotion.x;
				wmsg.mouseMotion.y = event.xmotion.y;
				break;
			case ClientMessage:
				if ((Atom) event.xclient.data.l[0] == ctx->wm_delete_window)
					wmsg.type = MTY_WINDOW_MSG_CLOSE;
				break;
		}

		if (wmsg.type != MTY_WINDOW_MSG_NONE)
			ctx->msg_func(&wmsg, (void *) ctx->opaque);
	}
}

void MTY_AppRun(MTY_App *app)
{
	do {
		app_events(app);

	} while (app->app_func(app->opaque));
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

bool MTY_AppIsActive(MTY_App *app)
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
	MTY_Window *ctx = MTY_Alloc(1, sizeof(MTY_Window));
	ctx->opaque = opaque;
	ctx->msg_func = msg_func;
	ctx->api = MTY_GFX_GL;

	bool r = true;

	Window root = XDefaultRootWindow(ctx->display);

	XSetWindowAttributes swa = {0};
	swa.colormap = XCreateColormap(ctx->display, root, vis->visual, AllocNone);
	swa.event_mask = WINDOW_MASK;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(ctx->display, root, &attr);

	uint32_t width = XWidthOfScreen(attr.screen);
	uint32_t height = XHeightOfScreen(attr.screen);

	ctx->window = XCreateWindow(ctx->display, root, 0, 0, width, height, 0, vis->depth,
		InputOutput, vis->visual, CWColormap | CWEventMask, &swa);

	XMapWindow(ctx->display, ctx->window);
	XMoveWindow(ctx->display, ctx->window, x, y);

	MTY_WindowSetTitle(ctx, title, NULL);

	// Resister custom messages from the window manager
	ctx->wm_delete_window = XInternAtom(ctx->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(ctx->display, ctx->window, &ctx->wm_delete_window, 1);

	except:

	if (!r)
		MTY_WindowDestroy(&ctx);

	return ctx;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
	XDestroyWindow(app->display, xw);
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	XStoreName(ctx->display, ctx->window, title);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	XWindowAttributes attr = {0};
	XGetWindowAttributes(ctx->display, ctx->window, &attr);

	*width = attr.width;
	*height = attr.height;

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	XWindowAttributes attr = {0};
	XGetWindowAttributes(ctx->display, window, &attr);

	*width = XWidthOfScreen(attr.screen);
	*height = XHeightOfScreen(attr.screen);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return ctx->dpi;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	if (fullscreen) {
		Hints hints = {0};
		hints.flags = 2;
		Atom prop = XInternAtom(ctx->display, "_MOTIF_WM_HINTS", True);

		XChangeProperty(ctx->display, ctx->window, prop, prop, 32, PropModeReplace, (unsigned char *) &hints, 5);
		XMapWindow(ctx->display, ctx->window);
	}
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return true;
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
	int revert = 0;
	Window w = 0;

	XGetInputFocus(ctx->display, &w, &revert);

	return w == ctx->window;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return window == 0;
}

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	return MTY_GFX_NONE;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	return NULL;
}

void *MTY_GLGetProcAddress(const char *name)
{
	if (!x_dl_global_init())
		return NULL;

	return glXGetProcAddress((const GLubyte *) name);
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

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}
