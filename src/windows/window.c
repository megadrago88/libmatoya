// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <math.h>

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shellscalingapi.h>

#include "mty-tls.h"
#include "hid/hid.h"

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_gl_)
GFX_CTX_PROTOTYPES(_d3d9_)
GFX_CTX_PROTOTYPES(_d3d11_)
GFX_CTX_DECLARE_TABLE()

#define WINDOW_CLASS_NAME L"MTY_Window"
#define WINDOW_RI_MAX     (32 * 1024)

struct window {
	MTY_Window window;
	MTY_MsgFunc func;
	MTY_GFX api;
	const void *opaque;
	HWND hwnd;
	uint32_t wx;
	uint32_t wy;
	uint32_t wwidth;
	uint32_t wheight;
	uint32_t width;
	uint32_t height;
	uint32_t min_width;
	uint32_t min_height;
	struct gfx_ctx *gfx_ctx;
	MTY_Renderer *renderer;
	RAWINPUT *ri;
};

struct app {
	bool init;
	WNDCLASSEX wc;
	ATOM class;
	UINT tb_msg;
	RECT clip;
	HICON cursor;
	HICON custom_cursor;
	HINSTANCE instance;
	HHOOK kbhook;
	HWND kbhwnd;
	bool pen_active;
	bool touch_active;
	bool relative;
	bool kbgrab;
	bool mgrab;
	bool default_cursor;
	bool ghk_disabled;
	bool filter_move;
	uint64_t prev_state;
	uint64_t state;
	int32_t last_x;
	int32_t last_y;
	MTY_Detach detach;
	MTY_Hash *hotkey;
	MTY_Hash *ghotkey;
	MTY_Hash *hid;
	MTY_Hash *hidid;

	struct window *windows[MTY_WINDOW_MAX];
	struct xip xinput[4];

	struct {
		HMENU menu;
		NOTIFYICONDATA nid;
		MTY_MenuItem *items;
		uint32_t len;
		int64_t ts;
		int64_t rctimer;
		bool init;
		bool want;
	} tray;
} APP;


// Weak Linking

static HRESULT (WINAPI *_GetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
static BOOL (WINAPI *_GetPointerType)(UINT32 pointerId, POINTER_INPUT_TYPE *pointerType);
static BOOL (WINAPI *_GetPointerPenInfo)(UINT32 pointerId, POINTER_PEN_INFO *penInfo);


// App Static Helpers

static HWND app_get_main_hwnd(void)
{
	if (!APP.windows[0])
		return NULL;

	return APP.windows[0]->hwnd;
}

static struct window *app_get_main_window(void)
{
	return APP.windows[0];
}

static struct window *app_get_window(MTY_Window window)
{
	return window < 0 ? NULL : APP.windows[window];
}

static struct window *app_get_focus_window(void)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (MTY_WindowIsActive(x))
			return app_get_window(x);

	return NULL;
}

static MTY_Window app_find_open_window(void)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (!APP.windows[x])
			return x;

	return -1;
}

static void app_hwnd_activate(HWND hwnd, bool active)
{
	if (active) {
		ShowWindow(hwnd, SW_RESTORE);
		SetForegroundWindow(hwnd);

	} else {
		ShowWindow(hwnd, SW_HIDE);
	}
}

static float app_hwnd_get_scale(HWND hwnd)
{
	if (_GetDpiForMonitor) {
		HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

		if (mon) {
			UINT x = 0;
			UINT y = 0;

			if (_GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &x, &y) == S_OK)
				return (float) x / 96.0f;
		}
	}

	return 1.0f;
}

static bool app_hwnd_visible(HWND hwnd)
{
	return IsWindowVisible(hwnd) && !IsIconic(hwnd);
}

static bool app_hwnd_active(HWND hwnd)
{
	return GetForegroundWindow() == hwnd && app_hwnd_visible(hwnd);
}

static void app_register_raw_input(USHORT usage_page, USHORT usage, DWORD flags, HWND hwnd)
{
	RAWINPUTDEVICE rid = {0};
	rid.usUsagePage = usage_page;
	rid.usUsage = usage;
	rid.dwFlags = flags;
	rid.hwndTarget = hwnd;
	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
		MTY_Log("'RegisterRawInputDevices' failed with error 0x%X", GetLastError());
}


// Hotkeys

static void app_unregister_global_hotkeys(void)
{
	HWND hwnd = app_get_main_hwnd();
	if (!hwnd)
		return;

	uint64_t i = 0;
	int64_t key = 0;

	while (MTY_HashNextKeyInt(APP.ghotkey, &i, &key))
		if (key > 0)
			UnregisterHotKey(hwnd, (int32_t) key);
}

static void app_kb_to_hotkey(MTY_Msg *wmsg)
{
	uint32_t hotkey = MTY_AppGetHotkey(wmsg->keyboard.mod, wmsg->keyboard.scancode);
	if (hotkey != 0) {
		if (wmsg->keyboard.pressed) {
			wmsg->type = MTY_WINDOW_MSG_HOTKEY;
			wmsg->hotkey = hotkey;
		} else {
			wmsg->type = MTY_WINDOW_MSG_NONE;
		}
	}
}

static bool app_scancode_to_str(MTY_Scancode scancode, char *str, size_t len)
{
	LONG lparam = (scancode & 0xFF) << 16;
	if (scancode & 0x100)
		lparam |= 1 << 24;

	wchar_t wstr[8] = {0};
	if (GetKeyNameText(lparam, wstr, 8))
		return MTY_WideToMulti(wstr, str, len);

	return false;
}

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
{
	memset(str, 0, len);

	strcat_s(str, len, (mod & MTY_KEYMOD_WIN) ? "Win+" : "");
	strcat_s(str, len, (mod & MTY_KEYMOD_CTRL) ? "Ctrl+" : "");
	strcat_s(str, len, (mod & MTY_KEYMOD_ALT) ? "Alt+" : "");
	strcat_s(str, len, (mod & MTY_KEYMOD_SHIFT) ? "Shift+" : "");

	if (scancode != MTY_SCANCODE_NONE) {
		char c[8] = {0};

		if (app_scancode_to_str(scancode, c, 8))
			strcat_s(str, len, c);
	}
}

void MTY_AppEnableGlobalHotkeys(bool enable)
{
	APP.ghk_disabled = !enable;
}

void MTY_AppSetHotkey(MTY_Hotkey mode, MTY_Keymod mod, MTY_Scancode scancode, uint32_t id)
{
	if (mode == MTY_HOTKEY_LOCAL) {
		MTY_HashSetInt(APP.hotkey, (mod << 16) | scancode, (void *) (uintptr_t) id);

	} else {
		HWND hwnd = app_get_main_hwnd();
		if (hwnd) {
			UINT wmod = MOD_NOREPEAT |
				((mod & MTY_KEYMOD_ALT) ? MOD_ALT : 0) |
				((mod & MTY_KEYMOD_CTRL) ? MOD_CONTROL : 0) |
				((mod & MTY_KEYMOD_SHIFT) ? MOD_SHIFT : 0) |
				((mod & MTY_KEYMOD_WIN) ? MOD_SHIFT : 0);

			UINT vk = MapVirtualKey(scancode & 0xFF, MAPVK_VSC_TO_VK);

			if (vk > 0) {
				uint32_t key = (wmod << 8) | vk;

				if (MTY_HashGetInt(APP.ghotkey, key) || id == 0)
					if (!UnregisterHotKey(hwnd, key))
						MTY_Log("'UnregisterHotKey' failed with error 0x%X", GetLastError());

				if (id > 0)
					if (!RegisterHotKey(hwnd, key, wmod, vk))
						MTY_Log("'RegisterHotKey' failed with error 0x%X", GetLastError());

				MTY_HashSetInt(APP.ghotkey, key, (void *) (uintptr_t) id);
			}
		}
	}
}

uint32_t MTY_AppGetHotkey(MTY_Keymod mod, MTY_Scancode scancode)
{
	return (uint32_t) (uintptr_t) MTY_HashGetInt(APP.hotkey, (mod << 16) | scancode);
}

void MTY_AppRemoveHotkeys(MTY_Hotkey mode)
{
	if (mode == MTY_HOTKEY_GLOBAL) {
		app_unregister_global_hotkeys();

		MTY_HashDestroy(&APP.ghotkey, NULL);
		APP.ghotkey = MTY_HashCreate(0);

	} else if (mode == MTY_HOTKEY_LOCAL) {
		MTY_HashDestroy(&APP.hotkey, NULL);
		APP.hotkey = MTY_HashCreate(0);
	}
}


// App Tray

#define APP_TRAY_CALLBACK (WM_USER + 510)
#define APP_TRAY_UID      1337
#define APP_TRAY_FIRST    1000

static HMENU app_tray_menu(MTY_MenuItem *items, uint32_t len, const void *opaque)
{
	HMENU menu = CreatePopupMenu();

	for (uint32_t x = 0; x < len; x++) {
		if (items[x].label[0]) {
			MENUITEMINFO item = {0};
			item.cbSize = sizeof(MENUITEMINFO);
			item.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
			item.wID = APP_TRAY_FIRST + x;
			item.dwTypeData = items[x].label;
			item.dwItemData = (ULONG_PTR) items[x].trayID;

			if (items[x].checked && items[x].checked((void *) opaque))
				item.fState |= MFS_CHECKED;

			InsertMenuItem(menu, APP_TRAY_FIRST + x, TRUE, &item);

		} else {
			InsertMenu(menu, APP_TRAY_FIRST + x, MF_SEPARATOR, TRUE, L"");
		}
	}

	return menu;
}

static bool app_tray_get_location(HWND hwnd, uint32_t *x, uint32_t *y)
{
	NOTIFYICONIDENTIFIER id = {0};
	id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	id.hWnd = hwnd;
	id.uID = APP_TRAY_UID;
	id.guidItem = GUID_NULL;

	RECT r = {0};
	if (Shell_NotifyIconGetRect(&id, &r) == S_OK) {
		if (x)
			*x = r.left + (r.right - r.left) / 2;

		if (y)
			*y = r.top + (r.bottom - r.top) / 2;

		return true;
	}

	return false;
}

static void app_tray_set_nid(NOTIFYICONDATA *nid, UINT uflags, HWND hwnd, HICON smicon, HICON lgicon)
{
	nid->cbSize = sizeof(NOTIFYICONDATA);
	nid->hWnd = hwnd;
	nid->uID = APP_TRAY_UID;
	nid->uFlags = NIF_MESSAGE | uflags;
	nid->uCallbackMessage = APP_TRAY_CALLBACK;
	nid->hIcon = smicon;
	nid->hBalloonIcon = lgicon;
	nid->dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;

	memset(nid->szInfoTitle, 0, sizeof(nid->szInfoTitle));
	memset(nid->szInfo, 0, sizeof(nid->szInfo));
}

static void app_tray_recreate(struct window *ctx)
{
	app_tray_set_nid(&APP.tray.nid, NIF_TIP | NIF_ICON, ctx->hwnd, APP.wc.hIconSm, APP.wc.hIcon);

	if (APP.tray.init)
		Shell_NotifyIcon(NIM_DELETE, &APP.tray.nid);

	APP.tray.init = Shell_NotifyIcon(NIM_ADD, &APP.tray.nid);
}

static void app_tray_retry(struct window *ctx)
{
	if (!APP.tray.init && APP.tray.want) {
		int64_t now = MTY_Timestamp();

		if (MTY_TimeDiff(APP.tray.rctimer, now) > 2000.0f) {
			app_tray_recreate(ctx);
			APP.tray.rctimer = now;
		}
	}
}

static void app_tray_msg(UINT msg, WPARAM wparam, LPARAM lparam, MTY_Msg *wmsg)
{
	struct window *ctx = app_get_main_window();
	if (!ctx)
		return;

	// Tray interaction
	if (msg == APP_TRAY_CALLBACK) {
		switch (lparam) {
			case NIN_BALLOONUSERCLICK:
				MTY_AppActivate(true);
				break;
			case WM_LBUTTONUP:
				int64_t now = MTY_Timestamp();

				if (MTY_TimeDiff(APP.tray.ts, now) > GetDoubleClickTime() * 2) {
					MTY_AppActivate(true);
					APP.tray.ts = now;
				}
				break;
			case WM_RBUTTONUP:
				if (APP.tray.menu)
					DestroyMenu(APP.tray.menu);

				APP.tray.menu = app_tray_menu(APP.tray.items, APP.tray.len, ctx->opaque);

				if (APP.tray.menu) {
					SetForegroundWindow(ctx->hwnd);
					SendMessage(ctx->hwnd, WM_INITMENUPOPUP, (WPARAM) APP.tray.menu, 0);

					uint32_t x = 0;
					uint32_t y = 0;
					if (app_tray_get_location(ctx->hwnd, &x, &y)) {
						WORD cmd = (WORD) TrackPopupMenu(APP.tray.menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON |
							TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, ctx->hwnd, NULL);

						SendMessage(ctx->hwnd, WM_COMMAND, cmd, 0);
						PostMessage(ctx->hwnd, WM_NULL, 0, 0);
					}
				}
				break;
		}

	// Recreation on taskbar recreation (DPI changes)
	} else if (msg == APP.tb_msg) {
		if (APP.tray.init)
			app_tray_recreate(ctx);

	// Menu command
	} else if (msg == WM_COMMAND && wparam >= APP_TRAY_FIRST && APP.tray.menu) {
		MENUITEMINFO item = {0};
		item.cbSize = sizeof(MENUITEMINFO);
		item.fMask = MIIM_ID | MIIM_DATA;

		if (GetMenuItemInfo(APP.tray.menu, (UINT) wparam, FALSE, &item)) {
			wmsg->type = MTY_WINDOW_MSG_TRAY;
			wmsg->trayID = (uint32_t) item.dwItemData;
		}
	}
}

void MTY_AppSetTray(const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
	HWND hwnd = app_get_main_hwnd();
	if (!hwnd)
		return;

	MTY_AppRemoveTray();

	APP.tray.want = true;
	APP.tray.rctimer = MTY_Timestamp();
	APP.tray.items = MTY_Dup(items, len * sizeof(MTY_MenuItem));
	APP.tray.len = len;

	app_tray_set_nid(&APP.tray.nid, NIF_TIP | NIF_ICON, hwnd, APP.wc.hIconSm, APP.wc.hIcon);
	MTY_MultiToWide(tooltip, APP.tray.nid.szTip, sizeof(APP.tray.nid.szTip) / sizeof(wchar_t));

	APP.tray.init = Shell_NotifyIcon(NIM_ADD, &APP.tray.nid);
	if (!APP.tray.init)
		MTY_Log("'Shell_NotifyIcon' failed");
}

void MTY_AppRemoveTray(void)
{
	HWND hwnd = app_get_main_hwnd();
	if (!hwnd)
		return;

	MTY_Free(APP.tray.items);
	APP.tray.want = false;
	APP.tray.items = NULL;
	APP.tray.len = 0;

	app_tray_set_nid(&APP.tray.nid, NIF_TIP | NIF_ICON, hwnd, APP.wc.hIconSm, APP.wc.hIcon);

	if (APP.tray.init)
		Shell_NotifyIcon(NIM_DELETE, &APP.tray.nid);

	APP.tray.init = false;
	memset(&APP.tray.nid, 0, sizeof(NOTIFYICONDATA));
}

void MTY_AppNotification(const char *title, const char *msg)
{
	if (!title && !msg)
		return;

	HWND hwnd = app_get_main_hwnd();
	if (!hwnd)
		return;

	app_tray_set_nid(&APP.tray.nid, NIF_INFO | NIF_REALTIME, hwnd, APP.wc.hIconSm, APP.wc.hIcon);

	if (title)
		MTY_MultiToWide(title, APP.tray.nid.szInfoTitle, sizeof(APP.tray.nid.szInfoTitle) / sizeof(wchar_t));

	if (msg)
		MTY_MultiToWide(msg, APP.tray.nid.szInfo, sizeof(APP.tray.nid.szInfo) / sizeof(wchar_t));

	Shell_NotifyIcon(NIM_MODIFY, &APP.tray.nid);
}


// Message processing

static LRESULT CALLBACK app_hwnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static void app_make_movement(HWND hwnd)
{
	POINT p = {0};
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	app_hwnd_proc(hwnd, WM_MOUSEMOVE, 0x1100, p.x | (p.y << 16));
}

static void app_ri_relative_mouse(HWND hwnd, const RAWINPUT *ri, MTY_Msg *wmsg)
{
	const RAWMOUSE *mouse = &ri->data.mouse;

	if (mouse->lLastX != 0 || mouse->lLastY != 0) {
		if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE) {
			int32_t x = mouse->lLastX;
			int32_t y = mouse->lLastY;

			// It seems that touch input reports lastX and lastY as screen coordinates,
			// not normalized coordinates between 0-65535 as the documentation says
			if (!APP.touch_active) {
				bool virt = (mouse->usFlags & MOUSE_VIRTUAL_DESKTOP) ? true : false;
				int32_t w = GetSystemMetrics(virt ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
				int32_t h = GetSystemMetrics(virt ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
				x = (int32_t) (((float) mouse->lLastX / 65535.0f) * w);
				y = (int32_t) (((float) mouse->lLastY / 65535.0f) * h);
			}

			if (APP.last_x != -1 && APP.last_y != -1) {
				wmsg->type = MTY_WINDOW_MSG_MOUSE_MOTION;
				wmsg->mouseMotion.relative = true;
				wmsg->mouseMotion.synth = true;
				wmsg->mouseMotion.x = (int32_t) (x - APP.last_x);
				wmsg->mouseMotion.y = (int32_t) (y - APP.last_y);
			}

			APP.last_x = x;
			APP.last_y = y;

		} else {
			wmsg->type = MTY_WINDOW_MSG_MOUSE_MOTION;
			wmsg->mouseMotion.relative = true;
			wmsg->mouseMotion.synth = false;
			wmsg->mouseMotion.x = mouse->lLastX;
			wmsg->mouseMotion.y = mouse->lLastY;
		}
	}

	ULONG b = mouse->usButtonFlags;

	if (b & RI_MOUSE_LEFT_BUTTON_DOWN)   app_hwnd_proc(hwnd, WM_LBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_LEFT_BUTTON_UP)     app_hwnd_proc(hwnd, WM_LBUTTONUP, 0, 0);
	if (b & RI_MOUSE_MIDDLE_BUTTON_DOWN) app_hwnd_proc(hwnd, WM_MBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_MIDDLE_BUTTON_UP)   app_hwnd_proc(hwnd, WM_MBUTTONUP, 0, 0);
	if (b & RI_MOUSE_RIGHT_BUTTON_DOWN)  app_hwnd_proc(hwnd, WM_RBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_RIGHT_BUTTON_UP)    app_hwnd_proc(hwnd, WM_RBUTTONUP, 0, 0);
	if (b & RI_MOUSE_BUTTON_4_DOWN)      app_hwnd_proc(hwnd, WM_XBUTTONDOWN, XBUTTON1 << 16, 0);
	if (b & RI_MOUSE_BUTTON_4_UP)        app_hwnd_proc(hwnd, WM_XBUTTONUP, XBUTTON1 << 16, 0);
	if (b & RI_MOUSE_BUTTON_5_DOWN)      app_hwnd_proc(hwnd, WM_XBUTTONDOWN, XBUTTON2 << 16, 0);
	if (b & RI_MOUSE_BUTTON_5_UP)        app_hwnd_proc(hwnd, WM_XBUTTONUP, XBUTTON2 << 16, 0);
	if (b & RI_MOUSE_WHEEL)              app_hwnd_proc(hwnd, WM_MOUSEWHEEL, mouse->usButtonData << 16, 0);
	if (b & RI_MOUSE_HWHEEL)             app_hwnd_proc(hwnd, WM_MOUSEHWHEEL, mouse->usButtonData << 16, 0);
}

static MTY_Keymod app_get_keymod(void)
{
	return
		((GetKeyState(VK_LSHIFT)   & 0x8000) >> 15) |
		((GetKeyState(VK_RSHIFT)   & 0x8000) >> 14) |
		((GetKeyState(VK_LCONTROL) & 0x8000) >> 13) |
		((GetKeyState(VK_RCONTROL) & 0x8000) >> 12) |
		((GetKeyState(VK_LMENU)    & 0x8000) >> 11) |
		((GetKeyState(VK_RMENU)    & 0x8000) >> 10) |
		((GetKeyState(VK_LWIN)     & 0x8000) >> 9)  |
		((GetKeyState(VK_RWIN)     & 0x8000) >> 8);
}

static LRESULT CALLBACK app_ll_keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		const KBDLLHOOKSTRUCT *p = (const KBDLLHOOKSTRUCT *) lParam;

		bool intercept = ((p->flags & LLKHF_ALTDOWN) && p->vkCode == VK_TAB) || // ALT + TAB
			(p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) || // Windows Key
			(p->vkCode == VK_APPS) || // Application Key
			(p->vkCode == VK_ESCAPE); // ESC

		if (intercept) {
			LPARAM wproc_lparam = 0;
			wproc_lparam |= (p->scanCode & 0xFF) << 16;
			wproc_lparam |= (p->flags & LLKHF_EXTENDED) ? (1 << 24) : 0;
			wproc_lparam |= (p->flags & LLKHF_UP) ? (1 << 31) : 0;

			SendMessage(APP.kbhwnd, (UINT) wParam, 0, wproc_lparam);
			return 1;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void app_apply_clip(bool focus)
{
	if (focus) {
		if (APP.relative && APP.detach != MTY_DETACH_FULL) {
			ClipCursor(&APP.clip);

		} else if (APP.mgrab && APP.detach == MTY_DETACH_NONE) {
			struct window *ctx = app_get_focus_window();
			if (ctx) {
				RECT r = {0};
				if (GetClientRect(ctx->hwnd, &r)) {
					ClientToScreen(ctx->hwnd, (POINT *) &r.left);
					ClientToScreen(ctx->hwnd, (POINT *) &r.right);
					ClipCursor(&r);
				}
			}
		} else {
			ClipCursor(NULL);
		}
	} else {
		ClipCursor(NULL);
	}
}

static void app_apply_cursor(bool focus)
{
	if (focus && APP.relative && APP.detach == MTY_DETACH_NONE) {
		APP.cursor = NULL;

	} else {
		APP.cursor = (APP.custom_cursor && !APP.default_cursor) ? APP.custom_cursor :
			LoadCursor(NULL, IDC_ARROW);
	}

	if (GetCursor() != APP.cursor)
		SetCursor(APP.cursor);
}

static void app_apply_mouse_ri(bool focus)
{
	if (APP.relative && !APP.pen_active) {
		if (focus) {
			if (APP.detach == MTY_DETACH_FULL) {
				app_register_raw_input(0x01, 0x02, 0, NULL);

			} else {
				app_register_raw_input(0x01, 0x02, RIDEV_NOLEGACY, NULL);
			}
		} else {
			app_register_raw_input(0x01, 0x02, 0, NULL);
		}
	} else {
		// Exiting raw input generates a single WM_MOUSEMOVE, filter it
		APP.filter_move = true;
		app_register_raw_input(0x01, 0x02, RIDEV_REMOVE, NULL);
	}
}

static void app_apply_keyboard_state(bool focus)
{
	if (focus && APP.kbgrab && APP.detach == MTY_DETACH_NONE) {
		if (!APP.kbhook) {
			struct window *ctx = app_get_focus_window();
			if (ctx) {
				APP.kbhwnd = ctx->hwnd;
				APP.kbhook = SetWindowsHookEx(WH_KEYBOARD_LL, app_ll_keyboard_proc, APP.instance, 0);
			}
		}

	} else {
		if (APP.kbhook) {
			UnhookWindowsHookEx(APP.kbhook);
			APP.kbhook = NULL;
			APP.kbhwnd = NULL;
		}
	}
}

static LRESULT app_custom_hwnd_proc(struct window *ctx, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	MTY_Msg wmsg = {0};
	wmsg.window = ctx->window;

	LRESULT r = 0;
	bool creturn = false;
	bool defreturn = false;
	char drop_name[MTY_PATH_MAX];

	switch (msg) {
		case WM_CLOSE:
			wmsg.type = MTY_WINDOW_MSG_CLOSE;
			break;
		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				app_apply_cursor(MTY_AppIsActive());
				creturn = true;
				r = TRUE;
			}
			break;
		case WM_DISPLAYCHANGE: {
			// A display change can bork DXGI ResizeBuffers, forcing a dummy 1px move puts it back to normal
			RECT wrect = {0};
			GetWindowRect(hwnd, &wrect);
			SetWindowPos(hwnd, 0, wrect.left + 1, wrect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			SetWindowPos(hwnd, 0, wrect.left, wrect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			break;
		}
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			wmsg.type = MTY_WINDOW_MSG_FOCUS;
			wmsg.focus = msg == WM_SETFOCUS;
			APP.state++;
			break;
		case WM_QUERYENDSESSION:
		case WM_ENDSESSION:
			wmsg.type = MTY_WINDOW_MSG_SHUTDOWN;
			break;
		case WM_GETMINMAXINFO: {
			float scale = app_hwnd_get_scale(hwnd);
			MINMAXINFO *info = (MINMAXINFO *) lparam;
			info->ptMinTrackSize.x = lrint((float) ctx->min_width * scale);
			info->ptMinTrackSize.y = lrint((float) ctx->min_height * scale);
			creturn = true;
			r = 0;
			break;
		}
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			wmsg.type = MTY_WINDOW_MSG_KEYBOARD;
			wmsg.keyboard.mod = app_get_keymod();
			wmsg.keyboard.pressed = !(lparam >> 31);
			wmsg.keyboard.scancode = lparam >> 16 & 0xFF;
			if (lparam >> 24 & 0x01)
				wmsg.keyboard.scancode |= 0x0100;
			break;
		case WM_MOUSEMOVE:
			if (!APP.filter_move && !APP.pen_active && (!APP.relative || app_hwnd_active(hwnd))) {
				wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
				wmsg.mouseMotion.relative = false;
				wmsg.mouseMotion.synth = false;
				wmsg.mouseMotion.x = GET_X_LPARAM(lparam);
				wmsg.mouseMotion.y = GET_Y_LPARAM(lparam);
				wmsg.mouseMotion.click = wparam == 0x1100; // Generated by MTY
			}

			APP.filter_move = false;
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			if (!APP.pen_active) {
				wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
				wmsg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT;
				wmsg.mouseButton.pressed = msg == WM_LBUTTONDOWN;
			}
			break;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if (!APP.pen_active) {
				wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
				wmsg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;
				wmsg.mouseButton.pressed = msg == WM_RBUTTONDOWN;
			}
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
			wmsg.mouseButton.button = MTY_MOUSE_BUTTON_MIDDLE;
			wmsg.mouseButton.pressed = msg == WM_MBUTTONDOWN;
			break;
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: {
			UINT button = GET_XBUTTON_WPARAM(wparam);
			wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
			wmsg.mouseButton.button = button == XBUTTON1 ? MTY_MOUSE_BUTTON_X1 :
				MTY_MOUSE_BUTTON_X2;
			wmsg.mouseButton.pressed = msg == WM_XBUTTONDOWN;
			break;
		}
		case WM_MOUSEWHEEL:
			wmsg.type = MTY_WINDOW_MSG_MOUSE_WHEEL;
			wmsg.mouseWheel.x = 0;
			wmsg.mouseWheel.y = GET_WHEEL_DELTA_WPARAM(wparam);
			break;
		case WM_MOUSEHWHEEL:
			wmsg.type = MTY_WINDOW_MSG_MOUSE_WHEEL;
			wmsg.mouseWheel.x = GET_WHEEL_DELTA_WPARAM(wparam);
			wmsg.mouseWheel.y = 0;
			break;
		case WM_POINTERENTER: {
			if (!_GetPointerType)
				break;

			UINT32 id = GET_POINTERID_WPARAM(wparam);

			POINTER_INPUT_TYPE type = PT_POINTER;
			if (_GetPointerType(id, &type)) {
				APP.pen_active = type == PT_PEN;
				APP.touch_active = type == PT_TOUCH || type == PT_TOUCHPAD;
			}
			break;
		}
		case WM_POINTERLEAVE:
			APP.pen_active = false;
			APP.touch_active = false;
			break;
		case WM_POINTERUPDATE:
			if (!_GetPointerType || !_GetPointerPenInfo)
				break;

			UINT32 id = GET_POINTERID_WPARAM(wparam);

			POINTER_INPUT_TYPE type = PT_POINTER;
			if (!_GetPointerType(id, &type) || type != PT_PEN)
				break;

			POINTER_PEN_INFO ppi = {0};
			if (!_GetPointerPenInfo(id, &ppi))
				break;

			POINTER_INFO *pi = &ppi.pointerInfo;
			wmsg.type = MTY_WINDOW_MSG_PEN;
			wmsg.pen.pressure = (uint16_t) ppi.pressure;
			wmsg.pen.rotation = (uint16_t) ppi.rotation;
			wmsg.pen.tiltX = (int8_t) ppi.tiltX;
			wmsg.pen.tiltY = (int8_t) ppi.tiltY;

			ScreenToClient(hwnd, &pi->ptPixelLocation);

			if (pi->ptPixelLocation.x < 0)
				pi->ptPixelLocation.x = 0;

			if (pi->ptPixelLocation.y < 0)
				pi->ptPixelLocation.y = 0;

			wmsg.pen.x = (uint16_t) pi->ptPixelLocation.x;
			wmsg.pen.y = (uint16_t) pi->ptPixelLocation.y;

			wmsg.pen.flags = 0;

			if (!(pi->pointerFlags & POINTER_FLAG_INRANGE))
				wmsg.pen.flags |= MTY_PEN_FLAG_LEAVE;

			if (pi->pointerFlags & POINTER_FLAG_INCONTACT)
				wmsg.pen.flags |= MTY_PEN_FLAG_TOUCHING;

			if (ppi.penFlags & PEN_FLAG_INVERTED)
				wmsg.pen.flags |= MTY_PEN_FLAG_INVERTED;

			if (ppi.penFlags & PEN_FLAG_ERASER)
				wmsg.pen.flags |= MTY_PEN_FLAG_ERASER;

			if (ppi.penFlags & PEN_FLAG_BARREL)
				wmsg.pen.flags |= MTY_PEN_FLAG_BARREL;

			defreturn = true;
			break;
		case WM_HOTKEY:
			if (!APP.ghk_disabled) {
				wmsg.type = MTY_WINDOW_MSG_HOTKEY;
				wmsg.hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(APP.ghotkey, (uint32_t) wparam);
			}
			break;
		case WM_CHAR: {
			wchar_t wstr[2] = {0};
			wstr[0] = (wchar_t) wparam;
			if (MTY_WideToMulti(wstr, wmsg.text, 8))
				wmsg.type = MTY_WINDOW_MSG_TEXT;
			break;
		}
		case WM_CLIPBOARDUPDATE:
			wmsg.type = MTY_WINDOW_MSG_CLIPBOARD;
			break;
		case WM_DROPFILES:
			wchar_t namew[MTY_PATH_MAX];

			if (DragQueryFile((HDROP) wparam, 0, namew, MTY_PATH_MAX)) {
				SetForegroundWindow(hwnd);

				MTY_WideToMulti(namew, drop_name, MTY_PATH_MAX);
				wmsg.drop.name = drop_name;
				wmsg.drop.data = MTY_ReadFile(drop_name, &wmsg.drop.size);

				if (wmsg.drop.data)
					wmsg.type = MTY_WINDOW_MSG_DROP;
			}
			break;
		case WM_INPUT:
			UINT rsize = WINDOW_RI_MAX;
			UINT e = GetRawInputData((HRAWINPUT) lparam, RID_INPUT, ctx->ri, &rsize, sizeof(RAWINPUTHEADER));
			if (e == 0 || e == 0xFFFFFFFF)
				break;

			RAWINPUTHEADER *header = &ctx->ri->header;
			if (header->dwType == RIM_TYPEMOUSE) {
				app_ri_relative_mouse(hwnd, ctx->ri, &wmsg);

			} else if (header->dwType == RIM_TYPEHID) {
				struct hid *hid = MTY_HashGetInt(APP.hid, (intptr_t) ctx->ri->header.hDevice);
				if (hid) {
					hid_state(hid, ctx->ri->data.hid.bRawData, ctx->ri->data.hid.dwSizeHid, &wmsg);

					// Prevent gamepad input while in the background
					if (wmsg.type == MTY_WINDOW_MSG_CONTROLLER && !MTY_AppIsActive())
						memset(&wmsg, 0, sizeof(MTY_Msg));
				}
			}
			break;
		case WM_INPUT_DEVICE_CHANGE:
			if (wparam == GIDC_ARRIVAL) {
				hid_xinput_refresh(APP.xinput);

				struct hid *hid = hid_create((HANDLE) lparam);
				if (hid) {
					wmsg.type = MTY_WINDOW_MSG_CONNECT;
					wmsg.controller.id = hid->id;
					wmsg.controller.vid = hid_get_vid(hid);
					wmsg.controller.pid = hid_get_pid(hid);

					hid_destroy(MTY_HashSetInt(APP.hid, lparam, hid));
					MTY_HashSetInt(APP.hidid, hid->id, hid);
				}

			} else if (wparam == GIDC_REMOVAL) {
				struct hid *hid = MTY_HashPopInt(APP.hid, lparam);
				if (hid) {
					wmsg.type = MTY_WINDOW_MSG_DISCONNECT;
					wmsg.controller.id = hid->id;
					wmsg.controller.vid = hid_get_vid(hid);
					wmsg.controller.pid = hid_get_pid(hid);

					MTY_HashPopInt(APP.hidid, hid->id);
					hid_destroy(hid);
				}
			}
			break;
	}

	// Tray
	app_tray_msg(msg, wparam, lparam, &wmsg);

	// Transform keyboard into hotkey
	if (wmsg.type == MTY_WINDOW_MSG_KEYBOARD)
		app_kb_to_hotkey(&wmsg);

	// For robustness, generate a WM_MOUSEMOVE on a mousedown
	if (wmsg.type == MTY_WINDOW_MSG_MOUSE_BUTTON && wmsg.mouseButton.pressed && !APP.relative)
		app_make_movement(hwnd);

	// Process the message
	if (wmsg.type != MTY_WINDOW_MSG_NONE) {
		ctx->func(&wmsg, (void *) ctx->opaque);

		if (wmsg.type == MTY_WINDOW_MSG_DROP)
			MTY_Free((void *) wmsg.drop.data);

		if (!defreturn)
			return creturn ? r : 0;
	}

	return creturn ? r : DefWindowProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK app_hwnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_NCCREATE:
			SetWindowLongPtr(hwnd, 0, (LONG_PTR) ((CREATESTRUCT *) lparam)->lpCreateParams);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			APP.state++;
			break;
		default:
			struct window *ctx = (struct window *) GetWindowLongPtr(hwnd, 0);
			if (ctx && hwnd)
				return app_custom_hwnd_proc(ctx, hwnd, msg, wparam, lparam);
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


// Clipboard

char *MTY_AppGetClipboard(void)
{
	char *text = NULL;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL)) {
		HGLOBAL clipbuffer = GetClipboardData(CF_UNICODETEXT);

		if (clipbuffer) {
			wchar_t *locked = GlobalLock(clipbuffer);

			if (locked) {
				text = MTY_WideToMultiD(locked);
				GlobalUnlock(clipbuffer);
			}
		}

		CloseClipboard();
	}

	return text;
}

void MTY_AppSetClipboard(const char *text)
{
	if (OpenClipboard(NULL)) {
		wchar_t *wtext = MTY_MultiToWideD(text);
		size_t size = (wcslen(wtext) + 1) * sizeof(wchar_t);

		HGLOBAL mem =  GlobalAlloc(GMEM_MOVEABLE, size);
		if (mem) {
			wchar_t *locked = GlobalLock(mem);

			if (locked) {
				if (EmptyClipboard()) {
					memcpy(locked, wtext, size);
					SetClipboardData(CF_UNICODETEXT, mem);
				}

				GlobalUnlock(mem);
			}

			GlobalFree(mem);
		}

		CloseClipboard();
	}
}


// Cursor

static void app_set_png_cursor(const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	HDC dc = NULL;
	ICONINFO ii = {0};
	void *mask = NULL;

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t *rgba = MTY_DecompressImage(image, size, &width, &height);
	if (!rgba)
		goto except;

	size_t pad = sizeof(size_t) * 8;
	size_t mask_len = width + ((pad - width % pad) / 8) * height;
	mask = MTY_Alloc(mask_len, 1);
	memset(mask, 0xFF, mask_len);

	dc = GetDC(NULL);
	if (!dc) {
		MTY_Log("'GetDC' failed");
		goto except;
	}

	BITMAPV4HEADER bi = {0};
	bi.bV4Size = sizeof(BITMAPV4HEADER);
	bi.bV4Width = width;
	bi.bV4Height = -((int32_t) height);
	bi.bV4Planes = 1;
	bi.bV4BitCount = 32;
	bi.bV4V4Compression = BI_BITFIELDS;
	bi.bV4AlphaMask = 0xFF000000;
	bi.bV4RedMask   = 0x00FF0000;
	bi.bV4GreenMask = 0x0000FF00;
	bi.bV4BlueMask  = 0x000000FF;

	uint8_t *mem = NULL;
	ii.xHotspot = hotX;
	ii.yHotspot = hotY;
	ii.hbmColor = CreateDIBSection(dc, (BITMAPINFO *) &bi, DIB_RGB_COLORS, &mem, NULL, 0);
	if (!ii.hbmColor) {
		MTY_Log("'CreateDIBSection' failed to create color bitmap");
		goto except;
	}

	ii.hbmMask = CreateBitmap(width, height, 1, 1, mask);
	if (!ii.hbmMask) {
		MTY_Log("'CreateBitmap' failed to create mask bitmap");
		goto except;
	}

	uint32_t pitch = width * 4;
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < pitch; x += 4) {
			mem[y * pitch + x + 0] = rgba[y * pitch + x + 2];
			mem[y * pitch + x + 1] = rgba[y * pitch + x + 1];
			mem[y * pitch + x + 2] = rgba[y * pitch + x + 0];
			mem[y * pitch + x + 3] = rgba[y * pitch + x + 3];
		}
	}

	APP.custom_cursor = CreateIconIndirect(&ii);
	if (!APP.custom_cursor) {
		MTY_Log("'CreateIconIndirect' failed with error 0x%X", GetLastError());
		goto except;
	}

	except:

	if (dc)
		ReleaseDC(NULL, dc);

	if (ii.hbmColor)
		DeleteObject(ii.hbmColor);

	if (ii.hbmMask)
		DeleteObject(ii.hbmMask);

	MTY_Free(mask);
	MTY_Free(rgba);
}

void MTY_AppSetPNGCursor(const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	if (APP.custom_cursor) {
		DestroyIcon(APP.custom_cursor);
		APP.custom_cursor = NULL;
		APP.state++;
	}

	if (image && size > 0) {
		app_set_png_cursor(image, size, hotX, hotY);
		APP.state++;
	}
}

void MTY_AppUseDefaultCursor(bool useDefault)
{
	if (APP.default_cursor != useDefault) {
		APP.default_cursor = useDefault;
		APP.state++;
	}
}


// App

bool MTY_AppCreate(void)
{
	if (APP.init)
		return false;

	bool r = true;

	APP.hotkey = MTY_HashCreate(0);
	APP.ghotkey = MTY_HashCreate(0);
	APP.hid = MTY_HashCreate(0);
	APP.hidid = MTY_HashCreate(0);
	APP.instance = GetModuleHandle(NULL);
	if (!APP.instance) {
		r = false;
		goto except;
	}

	APP.wc.cbSize = sizeof(WNDCLASSEX);
	APP.wc.cbWndExtra = sizeof(struct window *);
	APP.wc.lpfnWndProc = app_hwnd_proc;
	APP.wc.hInstance = APP.instance;
	APP.wc.lpszClassName = WINDOW_CLASS_NAME;

	APP.cursor = LoadCursor(NULL, IDC_ARROW);

	wchar_t path[MTY_PATH_MAX];
	GetModuleFileName(APP.instance, path, MTY_PATH_MAX);
	ExtractIconEx(path, 0, &APP.wc.hIcon, &APP.wc.hIconSm, 1);

	APP.class = RegisterClassEx(&APP.wc);
	if (APP.class == 0) {
		r = false;
		goto except;
	}

	APP.tb_msg = RegisterWindowMessage(L"TaskbarCreated");

	HMODULE user32 = GetModuleHandle(L"user32.dll");
	HMODULE shcore = GetModuleHandle(L"shcore.dll");
	_GetDpiForMonitor = (void *) GetProcAddress(shcore, "GetDpiForMonitor");
	_GetPointerPenInfo = (void *) GetProcAddress(user32, "GetPointerPenInfo");
	_GetPointerType = (void *) GetProcAddress(user32, "GetPointerType");

	hid_xinput_init();

	APP.init = true;

	except:

	if (!r)
		MTY_AppDestroy();

	return r;
}

void MTY_AppDestroy(void)
{
	if (!APP.init)
		return;

	if (APP.custom_cursor)
		DestroyIcon(APP.custom_cursor);

	MTY_AppRemoveTray();
	MTY_AppEnableScreenSaver(true);

	app_unregister_global_hotkeys();

	MTY_HashDestroy(&APP.hotkey, NULL);
	MTY_HashDestroy(&APP.ghotkey, NULL);
	MTY_HashDestroy(&APP.hid, hid_destroy);
	MTY_HashDestroy(&APP.hidid, NULL);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		MTY_WindowDestroy(x);

	if (APP.instance && APP.class != 0)
		UnregisterClass(WINDOW_CLASS_NAME, APP.instance);

	hid_xinput_destroy();

	memset(&APP, 0, sizeof(struct app));
}

void MTY_AppRun(MTY_AppFunc func, const void *opaque)
{
	for (bool cont = true; cont;) {
		struct window *window = app_get_main_window();
		if (!window)
			return;

		bool focus = MTY_AppIsActive();

		// Keyboard, mouse state changes
		if (APP.prev_state != APP.state) {
			app_apply_clip(focus);
			app_apply_cursor(focus);
			app_apply_mouse_ri(focus);
			app_apply_keyboard_state(focus);

			APP.prev_state = APP.state;
		}

		// XInput
		if (focus)
			hid_xinput_state(APP.xinput, window->window, window->func, (void *) window->opaque);

		// Tray retry in case of failure
		app_tray_retry(window);

		// Poll messages belonging to the current (main) thread
		for (MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		cont = func((void *) opaque);
	}
}

void MTY_AppDetach(MTY_Detach type)
{
	if (APP.detach != type) {
		APP.detach = type;
		APP.state++;
	}
}

MTY_Detach MTY_AppGetDetached(void)
{
	return APP.detach;
}

void MTY_AppEnableScreenSaver(bool enable)
{
	if (!SetThreadExecutionState(ES_CONTINUOUS | (!enable ? ES_DISPLAY_REQUIRED : 0)))
		MTY_Log("'SetThreadExecutionState' failed");
}

void MTY_AppGrabKeyboard(bool grab)
{
	if (APP.kbgrab != grab) {
		APP.kbgrab = grab;
		APP.state++;
	}
}

void MTY_AppGrabMouse(bool grab)
{
	if (APP.mgrab != grab) {
		APP.mgrab = grab;
		APP.state++;
	}
}

void MTY_AppSetRelativeMouse(bool relative)
{
	if (relative && !APP.relative) {
		APP.relative = true;
		APP.last_x = APP.last_y = -1;

		POINT p = {0};
		GetCursorPos(&p);
		APP.clip.left = p.x;
		APP.clip.right = p.x + 1;
		APP.clip.top = p.y;
		APP.clip.bottom = p.y + 1;

	} else if (!relative && APP.relative) {
		APP.relative = false;
		APP.last_x = APP.last_y = -1;
	}

	bool focus = MTY_AppIsActive();
	app_apply_mouse_ri(focus);
	app_apply_cursor(focus);
	app_apply_clip(focus);
}

bool MTY_AppGetRelativeMouse(void)
{
	return APP.relative;
}

void MTY_AppSetOnscreenKeyboard(bool enable)
{
}

void MTY_AppSetOrientation(MTY_Orientation orientation)
{
}

bool MTY_AppIsActive(void)
{
	bool r = false;

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		r = r || MTY_WindowIsActive(x);

	return r;
}

void MTY_AppActivate(bool active)
{
	HWND hwnd = app_get_main_hwnd();
	if (!hwnd)
		return;

	app_hwnd_activate(hwnd, active);
}

void MTY_AppControllerRumble(uint32_t id, uint16_t low, uint16_t high)
{
	if (id < 4) {
		hid_xinput_rumble(APP.xinput, id, low, high);

	} else {
		struct hid *hid = MTY_HashGetInt(APP.hidid, id);
		if (hid)
			hid_rumble(hid, low, high);
	}
}


// Window

static void window_client_to_full(uint32_t *width, uint32_t *height)
{
	RECT rect = {0};
	rect.right = *width;
	rect.bottom = *height;
	if (AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0)) {
		*width = rect.right - rect.left;
		*height = rect.bottom - rect.top;
	}
}

MTY_Window MTY_WindowCreate(const char *title, const MTY_WindowDesc *desc, MTY_MsgFunc func, const void *opaque)
{
	MTY_Window window = -1;
	wchar_t *titlew = NULL;
	bool r = true;

	if (!APP.init) {
		r = false;
		goto except;
	}

	window = app_find_open_window();
	if (window == -1) {
		r = false;
		goto except;
	}

	struct window *ctx = MTY_Alloc(1, sizeof(struct window));
	APP.windows[window] = ctx;

	ctx->opaque = opaque;
	ctx->func = func;
	ctx->ri = MTY_Alloc(WINDOW_RI_MAX, 1);
	ctx->window = window;
	ctx->min_width = desc->minWidth;
	ctx->min_height = desc->minHeight;

	RECT rect = {0};
	DWORD style = WS_OVERLAPPEDWINDOW;
	HWND desktop = GetDesktopWindow();
	uint32_t width = desc->width;
	uint32_t height = desc->height;

	GetWindowRect(desktop, &rect);
	int32_t desktop_height = rect.bottom - rect.top;
	int32_t desktop_width = rect.right - rect.left;

	float scale = app_hwnd_get_scale(desktop);
	int32_t x = lrint((float) desc->x * scale);
	int32_t y = lrint((float) desc->y * scale);

	if (desc->fullscreen) {
		style = WS_POPUP;
		x = rect.left;
		y = rect.top;
		width = desktop_width;
		height = desktop_height;

	} else {
		if (desc->creationHeight > 0.0f && (float) height * scale >
			desc->creationHeight * (float) desktop_height)
		{
			float aspect = (float) width / (float) height;
			height = lrint(desc->creationHeight * (float) desktop_height);
			width = lrint((float) height * aspect);

		} else {
			height = lrint((float) height * scale);
			width = lrint((float) width * scale);
		}

		window_client_to_full(&width, &height);

		if (desc->position == MTY_POSITION_CENTER) {
			x += (rect.right - width) / 2;
			y += (rect.bottom - height) / 2;
		}
	}

	titlew = MTY_MultiToWideD(title);

	ctx->hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, titlew, style,
		x, y, width, height, NULL, NULL, APP.instance, ctx);
	if (!ctx->hwnd) {
		r = false;
		goto except;
	}

	if (!desc->hidden)
		app_hwnd_activate(ctx->hwnd, true);

	DragAcceptFiles(ctx->hwnd, TRUE);

	if (window == 0) {
		AddClipboardFormatListener(ctx->hwnd);
		app_register_raw_input(0x01, 0x04, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, ctx->hwnd);
		app_register_raw_input(0x01, 0x05, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, ctx->hwnd);
		app_register_raw_input(0x01, 0x08, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, ctx->hwnd);
	}

	except:

	MTY_Free(titlew);

	if (!r) {
		MTY_WindowDestroy(window);
		window = -1;
	}

	return window;
}

void MTY_WindowDestroy(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	struct window *focus = app_get_focus_window();
	if (ctx == focus) {
		MTY_AppGrabKeyboard(false);
		MTY_AppGrabMouse(false);
	}

	if (ctx->hwnd)
		DestroyWindow(ctx->hwnd);

	MTY_Free(ctx->ri);
	MTY_Free(ctx);
	APP.windows[window] = NULL;
}

void MTY_WindowSetTitle(MTY_Window window, const char *title)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	wchar_t *titlew = MTY_MultiToWideD(title);
	SetWindowText(ctx->hwnd, titlew);
	MTY_Free(titlew);
}

bool MTY_WindowGetSize(MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	RECT rect = {0};
	if (GetClientRect(ctx->hwnd, &rect)) {
		*width = rect.right - rect.left;
		*height = rect.bottom - rect.top;
		return true;
	}

	return false;
}

static bool window_get_monitor_info(HWND hwnd, MONITORINFOEX *info)
{
	HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

	if (mon) {
		memset(info, 0, sizeof(MONITORINFOEX));
		info->cbSize = sizeof(MONITORINFOEX);

		return GetMonitorInfo(mon, (LPMONITORINFO) info);
	}

	return false;
}

bool MTY_WindowGetScreenSize(MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	MONITORINFOEX info = {0};
	if (window_get_monitor_info(ctx->hwnd, &info)) {
		*width = info.rcMonitor.right - info.rcMonitor.left;
		*height = info.rcMonitor.bottom - info.rcMonitor.top;
		return true;
	}

	return false;
}

float MTY_WindowGetScale(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return 1.0f;

	return app_hwnd_get_scale(ctx->hwnd);
}

void MTY_WindowEnableFullscreen(MTY_Window window, bool fullscreen)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	if (fullscreen && !MTY_WindowIsFullscreen(window)) {
		MONITORINFOEX info = {0};
		if (window_get_monitor_info(ctx->hwnd, &info)) {
			RECT r = {0};
			if (GetWindowRect(ctx->hwnd, &r)) {
				ctx->wx = r.left;
				ctx->wy = r.top;
			}

			MTY_WindowGetSize(window, &ctx->wwidth, &ctx->wheight);
			window_client_to_full(&ctx->wwidth, &ctx->wheight);

			uint32_t x = info.rcMonitor.left;
			uint32_t y = info.rcMonitor.top;
			uint32_t w = info.rcMonitor.right - info.rcMonitor.left;
			uint32_t h = info.rcMonitor.bottom - info.rcMonitor.top;

			SetWindowLongPtr(ctx->hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
			SetWindowPos(ctx->hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
		}

	} else if (!fullscreen && MTY_WindowIsFullscreen(window)) {
		SetWindowLongPtr(ctx->hwnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
		SetWindowPos(ctx->hwnd, HWND_TOP, ctx->wx, ctx->wy, ctx->wwidth, ctx->wheight, SWP_FRAMECHANGED);

		PostMessage(ctx->hwnd, WM_SETICON, ICON_BIG, GetClassLongPtr(ctx->hwnd, GCLP_HICON));
		PostMessage(ctx->hwnd, WM_SETICON, ICON_SMALL, GetClassLongPtr(ctx->hwnd, GCLP_HICONSM));
	}
}

bool MTY_WindowIsFullscreen(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	return GetWindowLongPtr(ctx->hwnd, GWL_STYLE) & WS_POPUP;
}

void MTY_WindowActivate(MTY_Window window, bool active)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	app_hwnd_activate(ctx->hwnd, active);
}

void MTY_WindowWarpCursor(MTY_Window window, uint32_t x, uint32_t y)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	POINT p;
	p.x = x;
	p.y = y;

	ClipCursor(NULL);
	if (ClientToScreen(ctx->hwnd, &p))
		SetCursorPos(p.x, p.y);

	MTY_AppSetRelativeMouse(false);
}

bool MTY_WindowIsVisible(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	return app_hwnd_visible(ctx->hwnd);
}

bool MTY_WindowIsActive(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	return app_hwnd_active(ctx->hwnd);
}

bool MTY_WindowExists(MTY_Window window)
{
	return app_get_window(window) ? true : false;
}


// Window GFX

void MTY_WindowPresent(MTY_Window window, uint32_t num_frames)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	GFX_CTX_API[ctx->api].gfx_ctx_present(ctx->gfx_ctx, num_frames);
}

MTY_Device *MTY_WindowGetDevice(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return NULL;

	return GFX_CTX_API[ctx->api].gfx_ctx_get_device(ctx->gfx_ctx);
}

MTY_Context *MTY_WindowGetContext(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return NULL;

	return GFX_CTX_API[ctx->api].gfx_ctx_get_context(ctx->gfx_ctx);
}

MTY_Texture *MTY_WindowGetBackBuffer(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return NULL;

	uint32_t width = 0;
	uint32_t height = 0;
	MTY_WindowGetSize(window, &width, &height);

	if (width != ctx->width || height != ctx->height) {
		if (GFX_CTX_API[ctx->api].gfx_ctx_refresh(ctx->gfx_ctx)) {
			ctx->width = width;
			ctx->height = height;
		}
	}

	return GFX_CTX_API[ctx->api].gfx_ctx_get_buffer(ctx->gfx_ctx);
}

void MTY_WindowDrawQuad(MTY_Window window, const void *image, const MTY_RenderDesc *desc)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	MTY_Texture *buffer = MTY_WindowGetBackBuffer(window);
	if (buffer) {
		MTY_RenderDesc mutated = *desc;
		mutated.viewWidth = ctx->width;
		mutated.viewHeight = ctx->height;

		MTY_Device *device = MTY_WindowGetDevice(window);
		MTY_Context *context = MTY_WindowGetContext(window);

		MTY_RendererDrawQuad(ctx->renderer, ctx->api, device, context, image, &mutated, buffer);
	}
}

void MTY_WindowDrawUI(MTY_Window window, const MTY_DrawData *dd)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	MTY_Texture *buffer = MTY_WindowGetBackBuffer(window);
	if (buffer) {
		MTY_Device *device = MTY_WindowGetDevice(window);
		MTY_Context *context = MTY_WindowGetContext(window);

		MTY_RendererDrawUI(ctx->renderer, ctx->api, device, context, dd, buffer);
	}
}

void MTY_WindowSetUITexture(MTY_Window window, uint32_t id, const void *rgba, uint32_t width, uint32_t height)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return;

	MTY_Device *device = MTY_WindowGetDevice(window);
	MTY_Context *context = MTY_WindowGetContext(window);

	MTY_RendererSetUITexture(ctx->renderer, ctx->api, device, context, id, rgba, width, height);
}

void *MTY_WindowGetUITexture(MTY_Window window, uint32_t id)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return NULL;

	return MTY_RendererGetUITexture(ctx->renderer, id);
}

bool MTY_WindowSetGFX(MTY_Window window, MTY_GFX api, bool vsync)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return false;

	MTY_RendererDestroy(&ctx->renderer);
	ctx->renderer = NULL;

	if (ctx->api != MTY_GFX_NONE)
		GFX_CTX_API[ctx->api].gfx_ctx_destroy(&ctx->gfx_ctx);

	if (api != MTY_GFX_NONE) {
		ctx->renderer = MTY_RendererCreate();
		ctx->gfx_ctx = GFX_CTX_API[api].gfx_ctx_create((void *) ctx->hwnd, vsync);

		if (!ctx->gfx_ctx && (api == MTY_GFX_D3D11 || api == MTY_GFX_D3D9))
			return MTY_WindowSetGFX(window, MTY_GFX_GL, vsync);

		ctx->api = api;
	}

	return ctx->gfx_ctx ? true : false;
}

MTY_GFX MTY_WindowGetGFX(MTY_Window window)
{
	struct window *ctx = app_get_window(window);
	if (!ctx)
		return MTY_GFX_NONE;

	return ctx->api;
}


// Misc

void *MTY_GLGetProcAddress(const char *name)
{
	void *p = wglGetProcAddress(name);

	if (!p)
		p = GetProcAddress(GetModuleHandleA("opengl32.dll"), name);

	return p;
}
