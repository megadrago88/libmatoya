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
#include "wsize.h"
#include "scancode.h"

struct window {
	Window window;
	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
	struct xpair pair;
};

struct MTY_App {
	Display *display;
	XVisualInfo *vis;
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	Atom wm_close;
	struct window *windows[MTY_WINDOW_MAX];
	uint32_t timeout;
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


// Window helpers

struct window *app_get_window(MTY_App *ctx, MTY_Window window)
{
	return window < 0 ? NULL : ctx->windows[window];
}

static MTY_Window app_find_open_window(MTY_App *ctx)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (!ctx->windows[x])
			return x;

	return -1;
}


// App

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

	bool r = true;
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display) {
		r = false;
		goto except;
	}

	GLint attr[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
	ctx->vis = glXChooseVisual(ctx->display, 0, attr);
	if (!ctx->vis) {
		r = false;
		goto except;
	}

	ctx->wm_close = XInternAtom(ctx->display, "WM_DELETE_WINDOW", False);

	const char *dpi = XGetDefault(ctx->display, "Xft", "dpi");
	ctx->dpi = dpi ? (float) atoi(dpi) / 96.0f : 1.0f;

	except:

	if (!r)
		MTY_AppDestroy(&ctx);

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	if (ctx->vis)
		XFree(ctx->vis);

	if (ctx->display)
		XCloseDisplay(ctx->display);

	MTY_Free(*app);
	*app = NULL;
}

static void app_event(MTY_App *ctx, XEvent *event)
{
	MTY_Msg msg = {0};

	switch (event->type) {
		case KeyPress:
		case KeyRelease: {
			msg.type = MTY_MSG_KEYBOARD;
			msg.keyboard.pressed = event->type == KeyPress;
			msg.keyboard.scancode = window_x_to_mty(XLookupKeysym(&event->xkey, 0));
			break;
		}
		case ButtonPress:
		case ButtonRelease:
			msg.type = MTY_MSG_MOUSE_BUTTON;
			msg.mouseButton.pressed = event->type == ButtonPress;

			switch (event->xbutton.button) {
				case 1: msg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT;   break;
				case 2: msg.mouseButton.button = MTY_MOUSE_BUTTON_MIDDLE; break;
				case 3: msg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;  break;

				// Mouse wheel
				case 4:
				case 5:
					msg.type = MTY_MSG_MOUSE_WHEEL;
					msg.mouseWheel.x = 0;
					msg.mouseWheel.y = event->xbutton.button == 4 ? -100 : 100;
					break;
			}
			break;
		case MotionNotify:
			msg.type = MTY_MSG_MOUSE_MOTION;
			msg.mouseMotion.relative = false;
			msg.mouseMotion.x = event->xmotion.x;
			msg.mouseMotion.y = event->xmotion.y;
			break;
		case ClientMessage:
			if ((Atom) event->xclient.data.l[0] == ctx->wm_close)
				msg.type = MTY_MSG_CLOSE;
			break;
	}

	if (msg.type != MTY_MSG_NONE)
		ctx->msg_func(&msg, ctx->opaque);
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true; cont;) {
		for (XEvent event; XEventsQueued(ctx->display, QueuedAlready) > 0;) {
			XNextEvent(ctx->display, &event);
			app_event(ctx, &event);
		}

		cont = ctx->app_func(ctx->opaque);

		if (ctx->timeout > 0)
			MTY_Sleep(ctx->timeout);
	}
}

void MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout)
{
	ctx->timeout = timeout;
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

bool MTY_AppIsActive(MTY_App *ctx)
{
	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(ctx->display, &w, &revert);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win && win->window == w)
			return true;
	}

	return false;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}


// Window

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	bool r = true;
	struct window *ctx = MTY_Alloc(1, sizeof(struct window));

	MTY_Window window = app_find_open_window(app);
	if (window == -1) {
		r = false;
		MTY_Log("Maximum windows (MTY_WINDOW_MAX) of %u reached", MTY_WINDOW_MAX);
		goto except;
	}

	app->windows[window] = ctx;

	Window root = XDefaultRootWindow(app->display);

	XSetWindowAttributes swa = {0};
	swa.colormap = XCreateColormap(app->display, root, app->vis->visual, AllocNone);
	swa.event_mask = WINDOW_MASK;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, root, &attr);

	int32_t desktop_width = XWidthOfScreen(attr.screen);
	int32_t desktop_height = XHeightOfScreen(attr.screen);
	int32_t width = desktop_width;
	int32_t height = desktop_height;
	int32_t x = lrint((float) desc->x * app->dpi);
	int32_t y = lrint((float) desc->y * app->dpi);

	wsize_client(desc, app->dpi, desktop_height, &x, &y, &width, &height);

	if (desc->position == MTY_POSITION_CENTER)
		wsize_center(0, 0, desktop_width, desktop_height, &x, &y, &width, &height);

	ctx->window = XCreateWindow(app->display, root, 0, 0, width, height, 0, app->vis->depth,
		InputOutput, app->vis->visual, CWColormap | CWEventMask, &swa);

	XMapWindow(app->display, ctx->window);
	XMoveWindow(app->display, ctx->window, x, y);

	MTY_WindowSetTitle(app, window, title);

	XSetWMProtocols(app->display, ctx->window, &app->wm_close, 1);

	ctx->pair.display = app->display;
	ctx->pair.vis = app->vis;
	ctx->pair.window = ctx->window;

	if (desc->api != MTY_GFX_NONE) {
		if (!MTY_WindowSetGFX(app, window, desc->api, desc->vsync)) {
			r = false;
			goto except;
		}
	}

	except:

	if (!r) {
		MTY_WindowDestroy(app, window);
		window = -1;
	}

	return window;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	XDestroyWindow(app->display, ctx->window);

	MTY_Free(ctx);
	app->windows[window] = NULL;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	XStoreName(app->display, ctx->window, title);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	*width = attr.width;
	*height = attr.height;

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	*width = XWidthOfScreen(attr.screen);
	*height = XHeightOfScreen(attr.screen);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return app->dpi;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (fullscreen) {
		Hints hints = {0};
		hints.flags = 2;
		Atom prop = XInternAtom(app->display, "_MOTIF_WM_HINTS", True);

		XChangeProperty(app->display, ctx->window, prop, prop, 32, PropModeReplace, (unsigned char *) &hints, 5);
		XMapWindow(app->display, ctx->window);
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
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(app->display, &w, &revert);

	return ctx->window == w;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return app_get_window(app, window) ? true : false;
}


// Window Private

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	ctx->api = api;
	ctx->gfx_ctx = gfx_ctx;
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return MTY_GFX_NONE;

	if (gfx_ctx)
		*gfx_ctx = ctx->gfx_ctx;

	return ctx->api;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return NULL;

	return (void *) &ctx->pair;
}


// Misc

void *MTY_GLGetProcAddress(const char *name)
{
	if (!x_dl_global_init())
		return NULL;

	return glXGetProcAddress((const GLubyte *) name);
}


// Unimplemented

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
