// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "scancode.h"

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
	struct gfx_ctx *gfx_ctx;
};


// Hotkeys

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
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


// Clipboard

char *MTY_AppGetClipboard(MTY_App *app)
{
	char *text = NULL;

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *available = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSPasteboardTypeString]];
	if ([available isEqualToString:NSPasteboardTypeString]) {
		NSString *string = [pasteboard stringForType:NSPasteboardTypeString];
		if (string)
			text = MTY_Strdup([string UTF8String]);
	}

	return text;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	app->cb_seq = [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:[NSString stringWithUTF8String:text] forType:NSPasteboardTypeString];
}


// Cursor

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	// https://developer.apple.com/documentation/uikit/uiimage/1624106-initwithdata?language=objc
	// https://developer.apple.com/documentation/appkit/nscursor/1524612-initwithimage?language=objc
	// [[NSCursor alloc] initWithImage:nsimage hotSpot:NSMakePoint(hot_x, hot_y)];
	// [NSCursor arrowCursor]
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}


// Event processing

static void app_events()
{
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
				wmsg.keyboard.scancode = keycode_to_scancode(event.keyCode);

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
}


// App

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, const void *opaque)
{
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp finishLaunching];

	return MTY_Alloc(1, 1);
}

void MTY_AppDestroy(MTY_App **app)
{
}

void MTY_AppRun(MTY_App *app)
{
	do {
		app_events();

	} while (app->app_func(opaque));
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	// IOPMAssertionCreateWithDescription
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

bool MTY_AppIsActive(MTY_App *app)
{
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

	MTY_Window ctx = 0;

	NSRect rect = NSMakeRect(0, 0, width, height);
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
	ctx->nswindow = [[MTYWindow alloc] initWithContentRect:rect styleMask:style
		backing:NSBackingStoreBuffered defer:NO];

	ctx->nswindow.title = [NSString stringWithUTF8String:title];
	[ctx->nswindow center];

	[ctx->nswindow makeKeyAndOrderFront:ctx->nswindow];

	except:

	if (!r) {
		MTY_WindowDestroy(app, ctx);
		ctx = -1;
	}

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
	// CGAssociateMouseAndMouseCursorPosition(YES) to prevent delay
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

void MTY_AppSetOnscreenKeyboard(MTY_App *app, bool enable)
{
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}
