// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "wsize.h"
#include "scancode.h"


// NSApp

@interface App : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>
	@property MTY_AppFunc app_func;
	@property MTY_MsgFunc msg_func;
	@property MTY_Hash *hotkey;
	@property MTY_Detach detach;
	@property void *opaque;
	@property bool relative;
	@property bool grab_mouse;
	@property bool default_cursor;
	@property bool cursor_outside;
	@property uint32_t cb_seq;
	@property NSCursor *custom_cursor;
	@property NSCursor *cursor;
	@property void **windows;
	@property IOPMAssertionID assertion;
@end

static void app_apply_cursor(App *ctx)
{
	NSCursor *arrow = [NSCursor arrowCursor];
	NSCursor *new = ctx.default_cursor || ctx.cursor_outside || ctx.detach != MTY_DETACH_NONE ? arrow :
		ctx.custom_cursor ? ctx.custom_cursor : arrow;

	if (ctx.cursor)
		[ctx.cursor pop];

	ctx.cursor = new;
	[ctx.cursor push];
}

static void app_add_menu_item(NSMenu *menu, NSString *title, NSString *key, SEL sel)
{
	NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:sel keyEquivalent:key];
	[menu addItem:item];
}

static void app_add_menu_separator(NSMenu *menu)
{
	[menu addItem:[NSMenuItem separatorItem]];
}

static void app_show_main_window(App *ctx)
{
	NSArray<NSWindow *> *windows = [NSApp windows];

	for (uint32_t x = 0; x < windows.count; x++) {
		if ((__bridge void *) windows[x] == ctx.windows[x]) {
			[windows[x] makeKeyAndOrderFront:ctx];
			break;
		}
	}
}

@implementation App : NSObject
	- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center
		shouldPresentNotification:(NSUserNotification *)notification
	{
		return YES;
	}

	- (void)applicationWillFinishLaunching:(NSNotification *)notification
	{
		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];

		[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
			andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass
			andEventID:kAEGetURL];
	}

	- (void)applicationDidUnhide:(NSNotification *)notification
	{
		[NSApp activateIgnoringOtherApps:YES];
		app_show_main_window(self);
	}

	- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
	{
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_QUIT;

		self.msg_func(&msg, self.opaque);

		return NSTerminateCancel;
	}

	- (void)appQuit:(id)sender
	{
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_QUIT;

		self.msg_func(&msg, self.opaque);
	}

	- (void)appClose:(id)sender
	{
		[[NSApp keyWindow] performClose:self];
	}

	- (void)appRestart:(id)sender
	{
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_TRAY;
		msg.trayID = 3;

		self.msg_func(&msg, self.opaque);
	}

	- (void)appMinimize:(id)sender
	{
		[[NSApp keyWindow] miniaturize:self];
	}

	- (void)controllerConnected
	{
		printf("CONNECT\n");
	}

	- (void)controllerDisconnected
	{
		printf("DISCONNECT\n");
	}

	- (void)applicationDidFinishLaunching:(NSNotification *)notification
	{
		// Activation policy of a regular app
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		// This makes the app show up in the dock and focusses it
		[NSApp activateIgnoringOtherApps:YES];

		// Main menu
		NSMenu *menubar = [NSMenu alloc];
		NSMenuItem *item = [NSMenuItem alloc];
		[menubar addItem:item];
		NSMenu *menu = [NSMenu alloc];

		app_add_menu_item(menu, @"Quit", @"q", @selector(appQuit:));
		app_add_menu_item(menu, @"Restart", @"", @selector(appRestart:));
		app_add_menu_separator(menu);
		app_add_menu_item(menu, @"Minimize", @"m", @selector(appMinimize:));
		app_add_menu_item(menu, @"Close", @"w", @selector(appClose:));

		[item setSubmenu:menu];
		[NSApp setMainMenu:menubar];
	}

	- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
	{
		if ([NSApp isHidden]) {
			[NSApp unhide:self];

		} else {
			[NSApp activateIgnoringOtherApps:YES];
			app_show_main_window(self);
		}

		return NO;
	}

	- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
	{
		NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];

		if (url) {
			MTY_Msg msg = {0};
			msg.type = MTY_MSG_REOPEN;
			msg.arg = [url UTF8String];

			self.msg_func(&msg, self.opaque);
		}
	}

	- (NSMenu *)applicationDockMenu:(NSApplication *)sender
	{
		NSMenu *menubar = [NSMenu alloc];
		NSMenuItem *item = [NSMenuItem alloc];
		[menubar addItem:item];
		NSMenu *menu = [NSMenu alloc];

		app_add_menu_item(menu, @"Restart", @"", @selector(appRestart:));

		[item setSubmenu:menu];
		return menu;
	}
@end


// NSWindow

@interface Window : NSWindow <NSWindowDelegate>
	@property(strong) App *app;
	@property MTY_Window window;
	@property MTY_GFX api;
	@property struct gfx_ctx *gfx_ctx;
@end

static Window *app_get_window(MTY_App *app, MTY_Window window)
{
	App *ctx = (__bridge App *) app;

	return window < 0 ? nil : (__bridge Window *) ctx.windows[window];
}

static MTY_Window app_find_open_window(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (!ctx.windows[x])
			return x;

	return -1;
}

static MTY_Msg window_msg(Window *window, MTY_MsgType type)
{
	MTY_Msg msg = {0};
	msg.type = type;
	msg.window = window.window;

	return msg;
}

static bool window_event_in_view(Window *window, NSPoint *p)
{
	*p = [window mouseLocationOutsideOfEventStream];
	NSSize size = window.contentView.frame.size;

	p->y = size.height - p->y;

	return p->x >= 0 && p->y >= 0 && p->x < size.width && p->y < size.height;
}

static Window *window_find_mouse(Window *me, NSPoint *p)
{
	Window *top = nil;
	bool key_hit = false;
	NSArray<NSWindow *> *windows = [NSApp windows];

	for (uint32_t x = 0; x < windows.count; x++) {
		Window *window = (Window *) windows[x];

		NSPoint wp = {0};
		if (window_event_in_view(window, &wp)) {
			if (window.isKeyWindow && window.windowNumber == me.windowNumber) {
				*p = wp;
				return window;
			}

			if (!top && !key_hit) {
				*p = wp;
				top = window;
			}

			if (window.isKeyWindow) {
				key_hit = true;
				top = nil;
			}
		}
	}

	return top;
}

static void window_mouse_event(Window *window, MTY_MouseButton button, bool pressed)
{
	NSPoint p = {0};
	Window *cur = window_find_mouse(window, &p);

	if (cur) {
		if (pressed && !cur.app.relative) {
			MTY_Msg msg = window_msg(cur, MTY_MSG_MOUSE_MOTION);
			CGFloat scale = cur.screen.backingScaleFactor;
			msg.mouseMotion.relative = false;
			msg.mouseMotion.click = true;
			msg.mouseMotion.x = lrint(scale * p.x);
			msg.mouseMotion.y = lrint(scale * p.y);

			window.app.msg_func(&msg, cur.app.opaque);
		}

		MTY_Msg msg = window_msg(cur, MTY_MSG_MOUSE_BUTTON);
		msg.mouseButton.pressed = pressed;
		msg.mouseButton.button = button;

		window.app.msg_func(&msg, window.app.opaque);

	}
}

static void window_warp_cursor(NSWindow *ctx, uint32_t x, int32_t y)
{
	CGFloat title_bar_h = ctx.frame.size.height - ctx.contentView.frame.size.height;
	CGFloat window_bottom = ctx.screen.frame.origin.y + ctx.frame.origin.y + ctx.frame.size.height;

	NSPoint pscreen = {0};
	CGFloat scale = ctx.screen.backingScaleFactor;
	pscreen.x = ctx.screen.frame.origin.x + ctx.frame.origin.x + (CGFloat) x / scale;
	pscreen.y = ctx.screen.frame.size.height - window_bottom + title_bar_h + (CGFloat) y / scale;

	CGWarpMouseCursorPosition(pscreen);
	CGAssociateMouseAndMouseCursorPosition(YES);
}

static void window_confine_cursor(void)
{
	NSWindow *window = [NSApp keyWindow];

	NSPoint wp = [window mouseLocationOutsideOfEventStream];
	NSSize size = window.contentView.frame.size;
	CGFloat scale = window.screen.backingScaleFactor;

	wp.y = size.height - wp.y;

	if (wp.x < 0)
		wp.x = 0;

	if (wp.y < 0)
		wp.y = 0;

	if (wp.x >= size.width)
		wp.x = size.width - 1;

	if (wp.y >= size.height)
		wp.y = size.height - 1;

	window_warp_cursor(window, lrint(wp.x * scale), lrint(wp.y * scale));
}

static void window_motion_event(Window *window, NSEvent *event)
{
	if (window.app.relative && window.app.detach == MTY_DETACH_NONE) {
		MTY_Msg msg = window_msg(window, MTY_MSG_MOUSE_MOTION);
		msg.mouseMotion.relative = true;
		msg.mouseMotion.x = event.deltaX;
		msg.mouseMotion.y = event.deltaY;

		window.app.msg_func(&msg, window.app.opaque);

	} else {
		NSPoint p = {0};
		Window *cur = window_find_mouse(window, &p);

		if (cur) {
			if (window.app.grab_mouse && window.app.detach != MTY_DETACH_NONE && !cur.isKeyWindow) {
				window_confine_cursor();

			} else {
				MTY_Msg msg = window_msg(cur, MTY_MSG_MOUSE_MOTION);

				CGFloat scale = cur.screen.backingScaleFactor;
				msg.mouseMotion.relative = false;
				msg.mouseMotion.x = lrint(scale * p.x);
				msg.mouseMotion.y = lrint(scale * p.y);

				window.app.msg_func(&msg, window.app.opaque);
			}

		} else if ([NSApp isActive] && window.app.grab_mouse && window.app.detach != MTY_DETACH_NONE) {
			window_confine_cursor();
		}
	}
}

static void window_keyboard_event(Window *window, int16_t key_code, NSEventModifierFlags flags, bool pressed)
{
	MTY_Msg msg = window_msg(window, MTY_MSG_KEYBOARD);
	msg.keyboard.scancode = keycode_to_scancode(key_code);
	msg.keyboard.mod = modifier_flags_to_keymod(flags);
	msg.keyboard.pressed = pressed;

	MTY_Keymod mod = msg.keyboard.mod & 0xFF;

	uint32_t hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(window.app.hotkey, (mod << 16) | msg.keyboard.scancode);

	if (hotkey != 0) {
		if (pressed) {
			msg.type = MTY_MSG_HOTKEY;
			msg.hotkey = hotkey;

			window.app.msg_func(&msg, window.app.opaque);
		}

	} else {
		if (msg.keyboard.scancode != MTY_SCANCODE_NONE)
			window.app.msg_func(&msg, window.app.opaque);
	}
}

@implementation Window : NSWindow
	- (BOOL)windowShouldClose:(NSWindow *)sender
	{
		MTY_Msg msg = window_msg(self, MTY_MSG_CLOSE);
		self.app.msg_func(&msg, self.app.opaque);

		return NO;
	}

	- (BOOL)canBecomeKeyWindow
	{
		return YES;
	}

	- (BOOL)canBecomeMainWindow
	{
		return YES;
	}

	- (BOOL)acceptsFirstResponder
	{
		return YES;
	}

	- (void)windowDidResignKey:(NSNotification *)notification
	{
		MTY_Msg msg = window_msg(self, MTY_MSG_FOCUS);
		msg.focus = false;

		self.app.msg_func(&msg, self.app.opaque);
	}

	- (void)windowDidBecomeKey:(NSNotification *)notification
	{
		MTY_Msg msg = window_msg(self, MTY_MSG_FOCUS);
		msg.focus = true;

		self.app.msg_func(&msg, self.app.opaque);
	}

	- (void)keyUp:(NSEvent *)event
	{
		window_keyboard_event(self, event.keyCode, event.modifierFlags, false);
	}

	- (void)keyDown:(NSEvent *)event
	{
		const char *text = [event.characters UTF8String];

		// Make sure visible ASCII
		if (text && text[0] && text[0] >= 0x20 && text[0] != 0x7F) {
			MTY_Msg msg = window_msg(self, MTY_MSG_TEXT);
			snprintf(msg.text, 8, "%s", text);

			self.app.msg_func(&msg, self.app.opaque);
		}

		window_keyboard_event(self, event.keyCode, event.modifierFlags, true);
	}

	- (void)flagsChanged:(NSEvent *)event
	{
		MTY_Msg msg = window_msg(self, MTY_MSG_KEYBOARD);
		msg.keyboard.scancode = keycode_to_scancode(event.keyCode);
		msg.keyboard.mod = modifier_flags_to_keymod(event.modifierFlags);

		switch (msg.keyboard.scancode) {
			case MTY_SCANCODE_LSHIFT: msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_LSHIFT; break;
			case MTY_SCANCODE_LCTRL:  msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_LCTRL;  break;
			case MTY_SCANCODE_LALT:   msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_LALT;   break;
			case MTY_SCANCODE_LWIN:   msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_LWIN;   break;
			case MTY_SCANCODE_RSHIFT: msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_RSHIFT; break;
			case MTY_SCANCODE_RCTRL:  msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_RCTRL;  break;
			case MTY_SCANCODE_RALT:   msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_RALT;   break;
			case MTY_SCANCODE_RWIN:   msg.keyboard.pressed = msg.keyboard.mod & MTY_KEYMOD_RWIN;   break;
			default:
				return;
		}

		self.app.msg_func(&msg, self.app.opaque);
	}

	- (void)mouseUp:(NSEvent *)event
	{
		window_mouse_event(self, MTY_MOUSE_BUTTON_LEFT, false);
	}

	- (void)mouseDown:(NSEvent *)event
	{
		window_mouse_event(self, MTY_MOUSE_BUTTON_LEFT, true);
	}

	- (void)rightMouseUp:(NSEvent *)event
	{
		window_mouse_event(self, MTY_MOUSE_BUTTON_RIGHT, false);
	}

	- (void)rightMouseDown:(NSEvent *)event
	{
		window_mouse_event(self, MTY_MOUSE_BUTTON_RIGHT, true);
	}

	- (void)otherMouseUp:(NSEvent *)event
	{
		if (event.buttonNumber == 2)
			window_mouse_event(self, MTY_MOUSE_BUTTON_MIDDLE, false);
	}

	- (void)otherMouseDown:(NSEvent *)event
	{
		if (event.buttonNumber == 2)
			window_mouse_event(self, MTY_MOUSE_BUTTON_MIDDLE, true);
	}

	- (void)mouseMoved:(NSEvent *)event
	{
		window_motion_event(self, event);
	}

	- (void)mouseDragged:(NSEvent *)event
	{
		window_motion_event(self, event);
	}

	- (void)rightMouseDragged:(NSEvent *)event
	{
		window_motion_event(self, event);
	}

	- (void)otherMouseDragged:(NSEvent *)event
	{
		window_motion_event(self, event);
	}

	- (void)mouseEntered:(NSEvent *)event
	{
		self.app.cursor_outside = false;
		app_apply_cursor(self.app);
	}

	- (void)mouseExited:(NSEvent *)event
	{
		self.app.cursor_outside = true;
		app_apply_cursor(self.app);
	}

	- (void)scrollWheel:(NSEvent *)event
	{
		CGFloat scale = self.screen.backingScaleFactor;
		int32_t delta = event.hasPreciseScrollingDeltas ? scale : scale * 80.0f;

		MTY_Msg msg = window_msg(self, MTY_MSG_MOUSE_WHEEL);
		msg.mouseWheel.x = lrint(event.scrollingDeltaX * delta);
		msg.mouseWheel.y = lrint(event.scrollingDeltaY * delta);

		self.app.msg_func(&msg, self.app.opaque);
	}
@end


// NSView

@interface View : NSView
	@property NSTrackingArea *area;
@end

@implementation View : NSView
	- (BOOL)acceptsFirstMouse:(NSEvent *)event
	{
		return YES;
	}

	- (void)updateTrackingAreas
	{
		if (_area)
			[self removeTrackingArea:_area];

		NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways;
		_area = [[NSTrackingArea alloc] initWithRect:self.bounds options:options owner:self.window userInfo:nil];

		[self addTrackingArea:_area];

		[super updateTrackingAreas];
	}
@end


// Hotkeys

static MTY_Atomic32 APP_GLOCK;
static char APP_KEYS[MTY_SCANCODE_MAX][16];

static void app_carbon_key(uint16_t kc, char *text, size_t len)
{
	TISInputSourceRef kb = TISCopyCurrentKeyboardInputSource();
	CFDataRef layout_data = TISGetInputSourceProperty(kb, kTISPropertyUnicodeKeyLayoutData);
	if (layout_data) {
		const UCKeyboardLayout *layout = (const UCKeyboardLayout *) CFDataGetBytePtr(layout_data);

		UInt32 dead_key_state = 0;
		UniCharCount out_len = 0;
		UniChar chars[8];

		UCKeyTranslate(layout, kc, kUCKeyActionDown, 0, LMGetKbdLast(),
			kUCKeyTranslateNoDeadKeysBit, &dead_key_state, 8, &out_len, chars);

		CFRelease(kb);

		NSString *ns_str = CFBridgingRelease(CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1));
		const char *c_str = [[ns_str uppercaseString] UTF8String];

		snprintf(text, len, "%s", c_str);
	}
}

void MTY_AppHotkeyToString(MTY_Keymod mod, MTY_Scancode scancode, char *str, size_t len)
{
	memset(str, 0, len);

	MTY_Strcat(str, len, (mod & MTY_KEYMOD_WIN) ? "Command+" : "");
	MTY_Strcat(str, len, (mod & MTY_KEYMOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_KEYMOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_KEYMOD_SHIFT) ? "Shift+" : "");

	if (scancode != MTY_SCANCODE_NONE) {
		if (MTY_Atomic32Get(&APP_GLOCK) == 0) {
			MTY_GlobalLock(&APP_GLOCK);

			dispatch_sync(dispatch_get_main_queue(), ^{
				for (uint16_t kc = 0; kc < 0x100; kc++) {
					MTY_Scancode sc = keycode_to_scancode(kc);

					if (sc != MTY_SCANCODE_NONE) {
						const char *text = keycode_to_text(kc);
						if (text) {
							snprintf(APP_KEYS[sc], 16, "%s", text);

						} else {
							app_carbon_key(kc, APP_KEYS[sc], 16);
						}
					}
				}
			});

			MTY_GlobalUnlock(&APP_GLOCK);
		}

		MTY_Strcat(str, len, APP_KEYS[scancode]);
	}
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Hotkey mode, MTY_Keymod mod, MTY_Scancode scancode, uint32_t id)
{
	App *ctx = (__bridge App *) app;

	mod &= 0xFF;

	if (mode == MTY_HOTKEY_LOCAL)
		MTY_HashSetInt(ctx.hotkey, (mod << 16) | scancode, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Hotkey mode)
{
	App *ctx = (__bridge App *) app;

	if (mode == MTY_HOTKEY_LOCAL) {
		MTY_Hash *h = ctx.hotkey;
		MTY_HashDestroy(&h, NULL);

		ctx.hotkey = MTY_HashCreate(0);
	}
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
	App *ctx = (__bridge App *) app;

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	ctx.cb_seq = [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:[NSString stringWithUTF8String:text] forType:NSPasteboardTypeString];
}


// Cursor

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	App *ctx = (__bridge App *) app;

	NSCursor *cursor = nil;

	if (image) {
		NSData *data = [NSData dataWithBytes:image length:size];
		NSImage *nsi = [[NSImage alloc] initWithData:data];

		cursor = [[NSCursor alloc] initWithImage:nsi hotSpot:NSMakePoint(hotX, hotY)];

	} else {
		if (ctx.custom_cursor)
			[ctx.custom_cursor pop];
	}

	ctx.custom_cursor = cursor;

	app_apply_cursor(ctx);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	App *ctx = (__bridge App *) app;

	ctx.default_cursor = useDefault;

	app_apply_cursor(ctx);

	MTY_AppSetRelativeMouse(app, false);
}


// App

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	App *ctx = [App new];
	ctx.app_func = appFunc;
	ctx.msg_func = msgFunc;
	ctx.opaque = opaque;

	ctx.windows = MTY_Alloc(MTY_WINDOW_MAX, sizeof(void *));
	ctx.hotkey = MTY_HashCreate(0);

	[NSApplication sharedApplication];
	[NSApp setDelegate:ctx];

	return (MTY_App *) CFBridgingRetain(ctx);
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	App *ctx = (App *) CFBridgingRelease(*app);
	for (int8_t x = 0; x < MTY_WINDOW_MAX; x++)
		MTY_WindowDestroy(*app, x);

	MTY_Hash *h = ctx.hotkey;
	MTY_HashDestroy(&h, NULL);
	ctx.hotkey = NULL;

	MTY_Free(ctx.windows);

	[NSApp terminate:ctx];
	ctx = nil;

	*app = NULL;
}

void MTY_AppRun(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	[NSApp finishLaunching];

	for (bool cont = true; cont;) {
		@autoreleasepool {
			while (true) {
				NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil
					inMode:NSDefaultRunLoopMode dequeue:YES];

				if (!event)
					break;

				[NSApp sendEvent:event];
			}

			cont = ctx.app_func(ctx.opaque);
		}
	}
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	App *ctx = (__bridge App *) app;

	ctx.detach = type;

	app_apply_cursor(ctx);

	if (ctx.relative) {
		if (ctx.detach == MTY_DETACH_FULL) {
			CGAssociateMouseAndMouseCursorPosition(YES);
			CGDisplayShowCursor(kCGDirectMainDisplay);

		} else {
			CGAssociateMouseAndMouseCursorPosition(NO);
			CGDisplayHideCursor(kCGDirectMainDisplay);
		}
	}
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	return ctx.detach;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	App *ctx = (__bridge App *) app;

	if (ctx.assertion) {
		IOPMAssertionRelease(ctx.assertion);
		ctx.assertion = 0;
	}

	if (enable) {
		IOPMAssertionID assertion = 0;
		IOPMAssertionCreateWithDescription(kIOPMAssertPreventUserIdleDisplaySleep, NULL,
			NULL, NULL, NULL, 0, NULL, &assertion);

		ctx.assertion = assertion;
	}
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
	App *ctx = (__bridge App *) app;

	ctx.grab_mouse = grab;
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	App *ctx = (__bridge App *) app;

	// NOTE: Argument is unused in CGDisplayXXXCursor functions

	if (ctx.detach != MTY_DETACH_FULL) {
		if (relative && !ctx.relative) {
			CGAssociateMouseAndMouseCursorPosition(NO);
			CGDisplayHideCursor(kCGDirectMainDisplay);

		} else if (!relative && ctx.relative) {
			CGAssociateMouseAndMouseCursorPosition(YES);
			CGDisplayShowCursor(kCGDirectMainDisplay);
		}

		ctx.relative = relative;
	}
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	return ctx.relative;
}

bool MTY_AppIsActive(MTY_App *app)
{
	return [NSApp isActive];
}

void MTY_AppActivate(MTY_App *app, bool active)
{
	App *ctx = (__bridge App *) app;

	if (active) {
		if ([NSApp isHidden]) {
			[NSApp unhide:ctx];

		} else {
			[NSApp activateIgnoringOtherApps:YES];
		}

	} else {
		[NSApp hide:ctx];
	}
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	// TODO
}


// Window

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	MTY_Window window = -1;
	bool r = true;

	Window *ctx = nil;
	View *content = nil;
	NSScreen *screen = nil;

	window = app_find_open_window(app);
	if (window == -1) {
		r = false;
		MTY_Log("Maximum windows (MTY_WINDOW_MAX) of %u reached", MTY_WINDOW_MAX);
		goto except;
	}

	screen = [NSScreen mainScreen];
	CGSize size = screen.frame.size;

	int32_t x = desc->x;
	int32_t y = -desc->y;
	int32_t width = size.width;
	int32_t height = size.height;

	wsize_client(desc, 1.0f, size.height, &x, &y, &width, &height);

	if (desc->position == MTY_POSITION_CENTER)
		wsize_center(0, 0, size.width, size.height, &x, &y, &width, &height);

	NSRect rect = NSMakeRect(x, y, width, height);
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
		NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	ctx = [[Window alloc] initWithContentRect:rect styleMask:style backing:NSBackingStoreBuffered defer:NO];
	ctx.window = window;
	ctx.app = (__bridge App *) app;

	[ctx setDelegate:ctx];
	[ctx setAcceptsMouseMovedEvents:YES];
	[ctx setReleasedWhenClosed:NO];
	[ctx setMinSize:NSMakeSize(desc->minWidth, desc->minHeight)];

	content = [[View alloc] initWithFrame:[ctx contentRectForFrameRect:ctx.frame]];
	[content setWantsBestResolutionOpenGLSurface:YES];
	[ctx setContentView:content];

	ctx.app.windows[window] = (__bridge void *) ctx;

	ctx.title = [NSString stringWithUTF8String:title];

	if (!desc->hidden)
		MTY_WindowActivate(app, window, true);

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
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	ctx.app.windows[window] = NULL;
	[ctx close];
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	NSString *nss = [NSString stringWithUTF8String:title];
	ctx.title = nss;
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	CGSize size = ctx.contentView.frame.size;
	CGFloat scale = ctx.screen.backingScaleFactor;

	*width = lrint(size.width * scale);
	*height = lrint(size.height * scale);

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	CGSize size = ctx.screen.frame.size;
	CGFloat scale = ctx.screen.backingScaleFactor;

	*width = lrint(size.width * scale);
	*height = lrint(size.height * scale);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return 1.0f;

	// macOS scales the display as though it switches resolutions,
	// so all we need to report is the high DPI device multiplier

	return ctx.screen.backingScaleFactor;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	bool is_fullscreen = MTY_WindowIsFullscreen(app, window);

	if ((!is_fullscreen && fullscreen) || (is_fullscreen && !fullscreen))
		[ctx toggleFullScreen:ctx];
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return ctx.styleMask & NSWindowStyleMaskFullScreen;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (active) {
		[ctx makeKeyAndOrderFront:ctx];

	} else {
		[ctx orderOut:ctx];
	}
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	window_warp_cursor(ctx, x, y);
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return ctx.isVisible;
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return ctx.isKeyWindow;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return app_get_window(app, window) ? true : false;
}


// Window Private

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	ctx.api = api;
	ctx.gfx_ctx = gfx_ctx;
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return MTY_GFX_NONE;

	if (gfx_ctx)
		*gfx_ctx = ctx.gfx_ctx;

	return ctx.api;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	Window *ctx = app_get_window(app, window);
	if (!ctx)
		return NULL;

	return (__bridge void *) ctx;
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

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}
