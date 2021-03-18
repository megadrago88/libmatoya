// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "app.h"

void MTY_HotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
}

uint32_t MTY_AppGetHotkey(MTY_App *app, MTY_Scope scope, MTY_Mod mod, MTY_Key key)
{
	return 0;
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Scope scope, MTY_Mod mod, MTY_Key key, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Scope scope)
{
}


// Clipboard

char *MTY_AppGetClipboard(MTY_App *app)
{
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
}


// Cursor

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

void MTY_AppShowCursor(MTY_App *app, bool show)
{
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}


// App

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_EventFunc eventFunc, void *opaque)
{
	return NULL;
}

void MTY_AppDestroy(MTY_App **app)
{
}

void MTY_AppRun(MTY_App *app)
{
}

void MTY_AppSetTimeout(MTY_App *app, uint32_t timeout)
{
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

bool MTY_AppMouseIsGrabbed(MTY_App *app)
{
	return false;
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
	return false;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

bool MTY_AppPenIsEnabled(MTY_App *app)
{
	return false;
}

void MTY_AppEnablePen(MTY_App *app, bool enable)
{
}


// Window

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return -1;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return false;
}

void MTY_WindowGetPosition(MTY_App *app, MTY_Window window, int32_t *x, int32_t *y)
{
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return false;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return 1.0f;
}

void MTY_WindowSetFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
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
	return false;
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return false;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return false;
}


// Window Private

void mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
}

MTY_GFX mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	return MTY_GFX_NONE;
}

void *mty_window_get_native(MTY_App *app, MTY_Window window)
{
	return NULL;
}


// Misc

void MTY_SetAppID(const char *id)
{
}


// Unimplemented

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

void MTY_AppShowSoftKeyboard(MTY_App *app, bool show)
{
}

bool MTY_AppSoftKeyboardIsShowing(MTY_App *app)
{
	return false;
}

MTY_Orientation MTY_AppGetOrientation(MTY_App *ctx)
{
	return MTY_ORIENTATION_USER;
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

bool MTY_AppKeyboardIsGrabbed(MTY_App *app)
{
	return false;
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

MTY_GFXState MTY_WindowGFXState(MTY_App *app, MTY_Window window)
{
	return MTY_GFX_STATE_NORMAL;
}

MTY_Input MTY_AppGetInputMode(MTY_App *ctx)
{
	return MTY_INPUT_UNSPECIFIED;
}

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
}
