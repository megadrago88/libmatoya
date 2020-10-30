// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

@interface MTYWindow : NSWindow
	- (BOOL)windowShouldClose:(NSWindow *)sender;
@end

@implementation MTYWindow : NSWindow
	bool closed = false;

	- (BOOL)windowShouldClose:(NSWindow *)sender
	{
		closed = true;
		return NO;
	}

	- (bool)wasClosed
	{
		bool closed = _closed;
		closed = false;

		return closed;
	}
@end

struct MTY_Window {
	MTYWindow *nswindow;
	MTY_MsgFunc msg_func;
	const void *opaque;

	bool relative;

	MTY_GFX api;
	float width;
	float height;
	struct gfx_ctx *gfx_ctx;

	MTY_Renderer *renderer;
};

static MTY_Scancode window_keycode_to_wmsg(unsigned short kc)
{
	switch (kc) {
		case kVK_ANSI_A: return MTY_SCANCODE_A;
		case kVK_ANSI_S: return MTY_SCANCODE_S;
		case kVK_ANSI_D: return MTY_SCANCODE_D;
		case kVK_ANSI_F: return MTY_SCANCODE_NONE;
		case kVK_ANSI_H: return MTY_SCANCODE_NONE;
		case kVK_ANSI_G: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Z: return MTY_SCANCODE_NONE;
		case kVK_ANSI_X: return MTY_SCANCODE_NONE;
		case kVK_ANSI_C: return MTY_SCANCODE_NONE;
		case kVK_ANSI_V: return MTY_SCANCODE_NONE;
		case kVK_ANSI_B: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Q: return MTY_SCANCODE_NONE;
		case kVK_ANSI_W: return MTY_SCANCODE_W;
		case kVK_ANSI_E: return MTY_SCANCODE_NONE;
		case kVK_ANSI_R: return MTY_SCANCODE_R;
		case kVK_ANSI_Y: return MTY_SCANCODE_NONE;
		case kVK_ANSI_T: return MTY_SCANCODE_T;
		case kVK_ANSI_1: return MTY_SCANCODE_NONE;
		case kVK_ANSI_2: return MTY_SCANCODE_NONE;
		case kVK_ANSI_3: return MTY_SCANCODE_NONE;
		case kVK_ANSI_4: return MTY_SCANCODE_NONE;
		case kVK_ANSI_6: return MTY_SCANCODE_NONE;
		case kVK_ANSI_5: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Equal: return MTY_SCANCODE_NONE;
		case kVK_ANSI_9: return MTY_SCANCODE_NONE;
		case kVK_ANSI_7: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Minus: return MTY_SCANCODE_NONE;
		case kVK_ANSI_8: return MTY_SCANCODE_NONE;
		case kVK_ANSI_0: return MTY_SCANCODE_NONE;
		case kVK_ANSI_RightBracket: return MTY_SCANCODE_NONE;
		case kVK_ANSI_O: return MTY_SCANCODE_O;
		case kVK_ANSI_U: return MTY_SCANCODE_NONE;
		case kVK_ANSI_LeftBracket: return MTY_SCANCODE_NONE;
		case kVK_ANSI_I: return MTY_SCANCODE_NONE;
		case kVK_ANSI_P: return MTY_SCANCODE_NONE;
		case kVK_ANSI_L: return MTY_SCANCODE_L;
		case kVK_ANSI_J: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Quote: return MTY_SCANCODE_NONE;
		case kVK_ANSI_K: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Semicolon: return MTY_SCANCODE_SEMICOLON;
		case kVK_ANSI_Backslash: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Comma: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Slash: return MTY_SCANCODE_NONE;
		case kVK_ANSI_N: return MTY_SCANCODE_NONE;
		case kVK_ANSI_M: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Period: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Grave: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadDecimal: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadMultiply: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadPlus: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadClear: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadDivide: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadEnter: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadMinus: return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadEquals: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad0: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad1: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad2: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad3: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad4: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad5: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad6: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad7: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad8: return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad9: return MTY_SCANCODE_NONE;
		case kVK_Return: return MTY_SCANCODE_ENTER;
		case kVK_Tab: return MTY_SCANCODE_NONE;
		case kVK_Space: return MTY_SCANCODE_SPACE;
		case kVK_Delete: return MTY_SCANCODE_NONE;
		case kVK_Escape: return MTY_SCANCODE_ESCAPE;
		case kVK_Command: return MTY_SCANCODE_NONE;
		case kVK_Shift: return MTY_SCANCODE_LSHIFT;
		case kVK_CapsLock: return MTY_SCANCODE_NONE;
		case kVK_Option: return MTY_SCANCODE_NONE;
		case kVK_Control: return MTY_SCANCODE_LCTRL;
		case kVK_RightShift: return MTY_SCANCODE_RSHIFT;
		case kVK_RightOption: return MTY_SCANCODE_NONE;
		case kVK_RightControl: return MTY_SCANCODE_RCTRL;
		case kVK_Function: return MTY_SCANCODE_NONE;
		case kVK_F17: return MTY_SCANCODE_NONE;
		case kVK_VolumeUp: return MTY_SCANCODE_NONE;
		case kVK_VolumeDown: return MTY_SCANCODE_NONE;
		case kVK_Mute: return MTY_SCANCODE_NONE;
		case kVK_F18: return MTY_SCANCODE_NONE;
		case kVK_F19: return MTY_SCANCODE_NONE;
		case kVK_F20: return MTY_SCANCODE_NONE;
		case kVK_F5: return MTY_SCANCODE_NONE;
		case kVK_F6: return MTY_SCANCODE_NONE;
		case kVK_F7: return MTY_SCANCODE_NONE;
		case kVK_F3: return MTY_SCANCODE_NONE;
		case kVK_F8: return MTY_SCANCODE_NONE;
		case kVK_F9: return MTY_SCANCODE_NONE;
		case kVK_F11: return MTY_SCANCODE_NONE;
		case kVK_F13: return MTY_SCANCODE_NONE;
		case kVK_F16: return MTY_SCANCODE_NONE;
		case kVK_F14: return MTY_SCANCODE_NONE;
		case kVK_F10: return MTY_SCANCODE_NONE;
		case kVK_F12: return MTY_SCANCODE_NONE;
		case kVK_F15: return MTY_SCANCODE_NONE;
		case kVK_Help: return MTY_SCANCODE_NONE;
		case kVK_Home: return MTY_SCANCODE_NONE;
		case kVK_PageUp: return MTY_SCANCODE_NONE;
		case kVK_ForwardDelete: return MTY_SCANCODE_NONE;
		case kVK_F4: return MTY_SCANCODE_NONE;
		case kVK_End: return MTY_SCANCODE_NONE;
		case kVK_F2: return MTY_SCANCODE_NONE;
		case kVK_PageDown: return MTY_SCANCODE_NONE;
		case kVK_F1: return MTY_SCANCODE_NONE;
		case kVK_LeftArrow: return MTY_SCANCODE_LEFT;
		case kVK_RightArrow: return MTY_SCANCODE_RIGHT;
		case kVK_DownArrow: return MTY_SCANCODE_DOWN;
		case kVK_UpArrow: return MTY_SCANCODE_UP;
		case kVK_ISO_Section: return MTY_SCANCODE_NONE;
		case kVK_JIS_Yen: return MTY_SCANCODE_NONE;
		case kVK_JIS_Underscore: return MTY_SCANCODE_NONE;
		case kVK_JIS_KeypadComma: return MTY_SCANCODE_NONE;
		case kVK_JIS_Eisu: return MTY_SCANCODE_NONE;
		case kVK_JIS_Kana: return MTY_SCANCODE_NONE;
		default:
			break;
	}

	return MTY_SCANCODE_NONE;
}

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Hotkey mode, MTY_Keymod mod, MTY_Scancode scancode, uint32_t id)
{
}

uint32_t MTY_AppGetHotkey(MTY_App *app, MTY_Keymod mod, MTY_Scancode scancode)
{
	return 0;
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Hotkey mode)
{
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

char *MTY_AppGetClipboard(void)
{
	// NSPasteboard
	return MTY_Alloc(1, 1);
}

void MTY_AppSetClipboard(const char *text)
{
	// NSPasteboard
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	// https://developer.apple.com/documentation/uikit/uiimage/1624106-initwithdata?language=objc
	// https://developer.apple.com/documentation/appkit/nscursor/1524612-initwithimage?language=objc
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, const void *opaque)
{
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp finishLaunching];

	return MTY_Alloc(1, 1);
}

void MTY_AppDestroy(MTY_App **app_)
{
}

void MTY_AppRun(MTY_App *app)
{
	do {
		CGSize size = ctx->nswindow.contentView.frame.size;
		uint32_t scale = lrint(ctx->nswindow.screen.backingScaleFactor);

		while (true) {
			NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
			if (!event)
				break;

			bool block_app = false;
			MTY_Msg wmsg = {0};

			switch (event.type) {
				case NSEventTypeKeyUp:
				case NSEventTypeKeyDown:
				case NSEventTypeFlagsChanged: {
					wmsg.keyboard.scancode = window_keycode_to_wmsg(event.keyCode);

					if (wmsg.keyboard.scancode != MTY_SCANCODE_NONE) {
						block_app = true;
						wmsg.type = MTY_WINDOW_MSG_KEYBOARD;

						if (event.type == NSEventTypeFlagsChanged) {
							switch (event.keyCode) {
								case kVK_Shift:         wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICELSHIFTKEYMASK;       break;
								case kVK_CapsLock:      wmsg.keyboard.pressed = event.modifierFlags & NSEventModifierFlagCapsLock;  break;
								case kVK_Option:        wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICELALTKEYMASK;         break;
								case kVK_Control:       wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICELCTLKEYMASK;         break;
								case kVK_RightShift:    wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICERSHIFTKEYMASK;       break;
								case kVK_RightOption:   wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICERALTKEYMASK;         break;
								case kVK_RightControl:  wmsg.keyboard.pressed = event.modifierFlags & NX_DEVICERCTLKEYMASK;         break;
								case kVK_Command:       wmsg.keyboard.pressed = event.modifierFlags & NX_COMMANDMASK;               break;
								default:
									block_app = false;
									wmsg.type = MTY_WINDOW_MSG_NONE;
									break;
							}
						} else {
							wmsg.keyboard.pressed = event.type == NSEventTypeKeyDown;
						}
					}
					break;
				}
				case NSEventTypeScrollWheel:
					wmsg.type = MTY_WINDOW_MSG_MOUSE_WHEEL;
					wmsg.mouseWheel.x = lrint(event.deltaX);
					wmsg.mouseWheel.y = lrint(event.deltaY);
					break;
				case NSEventTypeLeftMouseDown:
				case NSEventTypeLeftMouseUp:
				case NSEventTypeRightMouseDown:
				case NSEventTypeRightMouseUp:
				case NSEventTypeOtherMouseDown:
				case NSEventTypeOtherMouseUp:
					wmsg.type = MTY_WINDOW_MSG_MOUSE_BUTTON;
					wmsg.mouseButton.pressed = event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown ||
						event.type == NSEventTypeOtherMouseDown;

					switch (event.buttonNumber) {
						case 0: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_L; break;
						case 1: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_R; break;
					}
					break;
				case NSEventTypeLeftMouseDragged:
				case NSEventTypeRightMouseDragged:
				case NSEventTypeOtherMouseDragged:
				case NSEventTypeMouseMoved: {
					if (ctx->relative) {
						wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
						wmsg.mouseMotion.relative = true;
						wmsg.mouseMotion.x = event.deltaX;
						wmsg.mouseMotion.y = event.deltaY;

					} else {
						NSPoint p = [ctx->nswindow mouseLocationOutsideOfEventStream];
						int32_t x = lrint(p.x);
						int32_t y = lrint(size.height - p.y);

						if (x >= 0 && y >= 0 && x <= size.width && y <= size.height) {
							wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
							wmsg.mouseMotion.x = x * scale;
							wmsg.mouseMotion.y = y * scale;
						}
					}
					break;
				}
			}

			if ([ctx->nswindow wasClosed])
				wmsg.type = MTY_WINDOW_MSG_CLOSE;

			if (wmsg.type != MTY_WINDOW_MSG_NONE)
				ctx->msg_func(&wmsg, (void *) ctx->opaque);

			if (!block_app)
				[NSApp sendEvent:event];
		}

		size.width *= scale;
		size.height *= scale;

		if (size.width != ctx->width || size.height != ctx->height) {
			GFX_CTX_API[ctx->api].gfx_ctx_refresh(ctx->gfx_ctx);

			ctx->width = size.width;
			ctx->height = size.height;
		}
	} while (func(opaque));
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
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
	if (relative && !ctx->relative) {
		ctx->relative = true;
		CGAssociateMouseAndMouseCursorPosition(NO);
		CGDisplayHideCursor(kCGDirectMainDisplay);

	} else if (!relative && ctx->relative) {
		ctx->relative = false;
		CGAssociateMouseAndMouseCursorPosition(YES);
		CGDisplayShowCursor(kCGDirectMainDisplay);
	}
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
}

void MTY_AppSetOnscreenKeyboard(MTY_App *app, bool enable)
{
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

bool MTY_AppIsActive(MTY_App *app)
{
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	bool r = true;
	MTY_Window *ctx = MTY_Alloc(1, sizeof(MTY_Window));
	ctx->msg_func = msg_func;
	ctx->opaque = opaque;
	ctx->api = api == MTY_GFX_NONE ? MTY_GFX_METAL : api;

	NSRect rect = NSMakeRect(0, 0, width, height);
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
	ctx->nswindow = [[MTYWindow alloc] initWithContentRect:rect styleMask:style
		backing:NSBackingStoreBuffered defer:NO];

	ctx->nswindow.title = [NSString stringWithUTF8String:title];
	[ctx->nswindow center];

	[ctx->nswindow makeKeyAndOrderFront:ctx->nswindow];

	ctx->renderer = MTY_RendererCreate();

	ctx->gfx_ctx = GFX_CTX_API[ctx->api].gfx_ctx_create((__bridge void *) ctx->nswindow);
	if (!ctx->gfx_ctx) {
		r = false;
		goto except;
	}

	except:

	if (!r)
		MTY_WindowDestroy(&ctx);

	return ctx;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
	if (!window || !*window)
		return;

	MTY_Window *ctx = *window;

	ctx->nswindow = nil;

	MTY_Free(ctx);
	*window = NULL;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	NSString *nss = [NSString stringWithUTF8String:title];
	ctx->nswindow.title = nss;
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	CGSize size = ctx->nswindow.contentView.frame.size;
	CGFloat scale = ctx->nswindow.screen.backingScaleFactor;

	*width = lrint(size.width * scale);
	*height = lrint(size.height * scale);

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	// macOS scales the display as though it switches resolutions,
	// so all we need to report is the high DPI device multiplier

	return [ctx->nswindow screen].backingScaleFactor;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	bool is_fullscreen = MTY_WindowIsFullscreen(ctx);

	if ((!is_fullscreen && fullscreen) || (is_fullscreen && !fullscreen))
		[ctx->nswindow toggleFullScreen:ctx->nswindow];
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return [ctx->nswindow styleMask] & NSWindowStyleMaskFullScreen;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
	// CGWarpMouseCursorPosition
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return ctx->nswindow.isKeyWindow;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
}

void *MTY_GLGetProcAddress(const char *name)
{
}
