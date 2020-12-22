// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "x-dl.h"
#include "wsize.h"
#include "scancode.h"

struct window {
	Window window;
	XIC ic;

	bool fullscreen;
	XWindowAttributes wattr;

	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
	struct xinfo info;
};

struct MTY_App {
	Display *display;
	Cursor empty_cursor;
	Cursor custom_cursor;
	Cursor cursor;
	bool default_cursor;
	bool cursor_hidden;
	MTY_Detach detach;
	XVisualInfo *vis;
	Atom wm_close;
	XIM im;

	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Hash *hotkey;
	struct window *windows[MTY_WINDOW_MAX];
	uint32_t timeout;
	bool relative;
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

static MTY_Window app_find_window(MTY_App *ctx, Window xwindow)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win->window == xwindow)
			return x;
	}

	return 0;
}

static struct window *app_get_active_window(MTY_App *ctx)
{
	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(ctx->display, &w, &revert);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win && win->window == w)
			return win;
	}

	return NULL;
}

static MTY_Window app_get_active_index(MTY_App *ctx)
{
	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(ctx->display, &w, &revert);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win && win->window == w)
			return x;
	}

	return 0;
}


// App

static void __attribute__((destructor)) app_global_destroy(void)
{
	x_dl_global_destroy();
}

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
{
	// TODO
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Keymod mod, MTY_Scancode scancode, uint32_t id)
{
	mod &= 0xFF;
	MTY_HashSetInt(ctx->hotkey, (mod << 16) | scancode, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
	MTY_HashDestroy(&ctx->hotkey, NULL);
	ctx->hotkey = MTY_HashCreate(0);
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	// TODO
}

static void app_apply_cursor(MTY_App *app)
{
	Cursor cur = app->cursor_hidden ? app->empty_cursor :
		app->default_cursor || app->detach != MTY_DETACH_NONE ? None : app->custom_cursor ?
		app->custom_cursor : None;

	if (cur != app->cursor) {
		for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
			struct window *win = app_get_window(app, x);

			if (win)
				XDefineCursor(app->display, win->window, cur);
		}

		app->cursor = cur;
	}
}

static Cursor app_png_cursor(Display *display, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	Cursor cursor = None;
	XcursorImage *ximage = NULL;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t *rgba = MTY_DecompressImage(image, size, &width, &height);
	if (!rgba)
		goto except;

	ximage = XcursorImageCreate(width, height);
	if (!ximage)
		goto except;

	ximage->xhot = hotX;
	ximage->yhot = hotY;
	ximage->delay = 0;

	for (uint32_t i = 0; i < width * height; i++) {
		uint32_t p = rgba[i];
		uint32_t a = (p & 0x00FF0000) >> 16;
		uint32_t b = (p & 0x000000FF) << 16;;

		ximage->pixels[i] = (p & 0xFF00FF00) | a | b;
	}

	cursor = XcursorImageLoadCursor(display, ximage);

	except:

	if (ximage)
		XcursorImageDestroy(ximage);

	MTY_Free(rgba);

	return cursor;
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	if (app->cursor == app->custom_cursor)
		app->cursor = None;

	Cursor prev = app->custom_cursor;
	app->custom_cursor = None;

	if (image && size > 0)
		app->custom_cursor = app_png_cursor(app->display, image, size, hotX, hotY);

	app_apply_cursor(app);

	if (prev)
		XFreeCursor(app->display, prev);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	app->default_cursor = useDefault;
	app_apply_cursor(app);
}

static Cursor app_create_empty_cursor(Display *display)
{
	Cursor cursor = 0;

	char data[1] = {0};
	Pixmap p = XCreateBitmapFromData(display, XDefaultRootWindow(display), data, 1, 1);

	if (p) {
		XColor c = {0};
		cursor = XCreatePixmapCursor(display, p, p, &c, &c, 0, 0);

		XFreePixmap(display, p);
	}

	return cursor;
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	if (!x_dl_global_init())
		return NULL;

	XInitThreads();

	bool r = true;
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->hotkey = MTY_HashCreate(0);
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display) {
		r = false;
		goto except;
	}

	ctx->empty_cursor = app_create_empty_cursor(ctx->display);

	unsigned char mask[3] = {0};
	XISetMask(mask, XI_RawMotion);

	XIEventMask em = {0};
	em.deviceid = XIAllMasterDevices;
	em.mask_len = 3;
	em.mask = mask;

	XISelectEvents(ctx->display, XDefaultRootWindow(ctx->display), &em, 1);

	ctx->im = XOpenIM(ctx->display, NULL, NULL, NULL);
	if (!ctx->im) {
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

	if (ctx->empty_cursor)
		XFreeCursor(ctx->display, ctx->empty_cursor);

	if (ctx->vis)
		XFree(ctx->vis);

	if (ctx->im)
		XCloseIM(ctx->im);

	if (ctx->display)
		XCloseDisplay(ctx->display);

	MTY_HashDestroy(&ctx->hotkey, NULL);

	MTY_Free(*app);
	*app = NULL;
}

static void window_text_event(MTY_App *ctx, XEvent *event)
{
	struct window *win = app_get_window(ctx, 0);
	if (!win)
		return;

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_TEXT;

	Status status = 0;
	KeySym ks = 0;

	if (Xutf8LookupString(win->ic, (XKeyPressedEvent *) event, msg.text, 8, &ks, &status) > 0)
		ctx->msg_func(&msg, ctx->opaque);
}

static void app_event(MTY_App *ctx, XEvent *event)
{
	MTY_Msg msg = {0};

	switch (event->type) {
		case KeyPress:
			window_text_event(ctx, event);
			// Fall through

		case KeyRelease: {
			msg.type = MTY_MSG_KEYBOARD;
			msg.window = app_find_window(ctx, event->xkey.window);
			msg.keyboard.pressed = event->type == KeyPress;
			msg.keyboard.scancode = window_keysym_to_scancode(XLookupKeysym(&event->xkey, 0));
			// TODO Full scancode support
			break;
		}
		case ButtonPress:
		case ButtonRelease:
			msg.type = MTY_MSG_MOUSE_BUTTON;
			msg.window = app_find_window(ctx, event->xbutton.window);
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
					msg.mouseWheel.y = event->xbutton.button == 4 ? 100 : -100;
					break;
			}
			break;
		case MotionNotify:
			if (ctx->relative)
				break;

			msg.type = MTY_MSG_MOUSE_MOTION;
			msg.window = app_find_window(ctx, event->xmotion.window);
			msg.mouseMotion.relative = false;
			msg.mouseMotion.x = event->xmotion.x;
			msg.mouseMotion.y = event->xmotion.y;
			break;
		case ClientMessage:
			if ((Atom) event->xclient.data.l[0] == ctx->wm_close) {
				msg.window = app_find_window(ctx, event->xclient.window);
				msg.type = MTY_MSG_CLOSE;
			}
			break;
		case GenericEvent:
			if (!ctx->relative)
				break;

			if (XGetEventData(ctx->display, &event->xcookie)) {
				switch (event->xcookie.evtype) {
					case XI_RawMotion: {
						const XIRawEvent *re = (const XIRawEvent *) event->xcookie.data;
						const double *coords = (const double *) re->raw_values;

						msg.type = MTY_MSG_MOUSE_MOTION;
						msg.window = app_get_active_index(ctx);
						msg.mouseMotion.relative = true;
						msg.mouseMotion.x = lrint(coords[0]);
						msg.mouseMotion.y = lrint(coords[1]);
						break;
					}
				}
			}
			break;
	}

	if (msg.type != MTY_MSG_NONE)
		ctx->msg_func(&msg, ctx->opaque);
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true; cont;) {
		for (XEvent event; XEventsQueued(ctx->display, QueuedAfterFlush) > 0;) {
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
	app->detach = type;
	app_apply_cursor(app);
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	// TODO
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
	// TODO
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	// TODO This should keep track of the position where the cursor went
	// into relative

	if (!app->relative && relative) {
		struct window *win = app_get_active_window(app);

		if (win) {
			XGrabPointer(app->display, win->window, False,
				ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask,
				GrabModeAsync, GrabModeAsync, win->window, None, CurrentTime);

			app->cursor_hidden = true;
			app_apply_cursor(app);

			XSync(app->display, False);
		}

	} else if (app->relative && !relative) {
		XUngrabPointer(app->display, CurrentTime);

		app->cursor_hidden = false;
		app_apply_cursor(app);

		XSync(app->display, False);
	}

	app->relative = relative;
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return app->relative;
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return app_get_active_window(ctx) ? true : false;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
	// TODO
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	// TODO
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

	ctx->ic = XCreateIC(app->im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, ctx->window, NULL);

	XMapWindow(app->display, ctx->window);
	XMoveWindow(app->display, ctx->window, x, y);

	MTY_WindowSetTitle(app, window, title);

	XSetWMProtocols(app->display, ctx->window, &app->wm_close, 1);

	ctx->info.display = app->display;
	ctx->info.vis = app->vis;
	ctx->info.window = ctx->window;

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

	if (ctx->ic)
		XDestroyIC(ctx->ic);

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
	// FIXME This doesn't seem great
	return app->dpi;
}

static void window_set_borderless(Display *display, Window window, bool enable)
{
	Atom prop = XInternAtom(display, "_MOTIF_WM_HINTS", True);

	Hints hints = {0};
	hints.flags = 2;
	hints.decorations = enable ? 1 : 0;

	XChangeProperty(display, window, prop, prop, 32, PropModeReplace, (unsigned char *) &hints, 5);
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (fullscreen && !ctx->fullscreen) {
		window_set_borderless(app->display, ctx->window, false);

		XGetWindowAttributes(app->display, ctx->window, &ctx->wattr);

		uint32_t width = ctx->wattr.width;
		uint32_t height = ctx->wattr.height;
		MTY_WindowGetScreenSize(app, window, &width, &height);

		XResizeWindow(app->display, ctx->window, width, height);
		XMoveWindow(app->display, ctx->window, 0, 0);
		XMapWindow(app->display, ctx->window);
		XSync(app->display, False);

	} else if (!fullscreen && ctx->fullscreen) {
		window_set_borderless(app->display, ctx->window, true);

		XResizeWindow(app->display, ctx->window, ctx->wattr.width, ctx->wattr.height);
		XMoveWindow(app->display, ctx->window, ctx->wattr.x, ctx->wattr.y);
		XMapWindow(app->display, ctx->window);
		XSync(app->display, False);
	}

	ctx->fullscreen = fullscreen;
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return ctx->fullscreen;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
	// TODO
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	MTY_AppSetRelativeMouse(app, false);

	XWarpPointer(app->display, None, ctx->window, 0, 0, 0, 0, x, y);
	XSync(app->display, False);
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	return attr.map_state == IsViewable;
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

	return (void *) &ctx->info;
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

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}
