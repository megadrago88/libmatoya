// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "scancode.h"

enum {
	WINDOW_EVENT_CLOSE         = 1,
	WINDOW_EVENT_MOUSE_ENTERED = 2,
	WINDOW_EVENT_MOUSE_EXITED  = 3,
};

static void app_custom_event(int32_t window_num, int16_t type, int32_t d1, int32_t d2)
{
	NSEvent *event = [NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
		modifierFlags:0 timestamp:[[NSDate date] timeIntervalSince1970] windowNumber:window_num
		context:nil subtype:type data1:d1 data2:d2];

	[NSApp postEvent:event atStart:NO];
}


// NSView

@interface View : NSView
	@property NSTrackingArea *area;
@end

@implementation View : NSView
	- (BOOL)acceptsFirstMouse:(NSEvent *)event
	{
		return YES;
	}

	- (void)mouseEntered:(NSEvent *)event
	{
		app_custom_event(self.window.windowNumber, WINDOW_EVENT_MOUSE_ENTERED, 0, 0);

		[super mouseEntered:event];
	}

	- (void)mouseExited:(NSEvent *)event
	{
		app_custom_event(self.window.windowNumber, WINDOW_EVENT_MOUSE_EXITED, 0, 0);

		[super mouseExited:event];
	}

	- (void)updateTrackingAreas
	{
		if (_area)
			[self removeTrackingArea:_area];

		NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways;
		_area = [[NSTrackingArea alloc] initWithRect:self.bounds options:options owner:self userInfo:nil];

		[self addTrackingArea:_area];

		[super updateTrackingAreas];
	}
@end


// NSWindow

@interface Window : NSWindow <NSWindowDelegate>
	@property MTY_Window window;
	@property MTY_GFX api;
	@property struct gfx_ctx *gfx_ctx;
@end

@implementation Window : NSWindow
	- (BOOL)windowShouldClose:(NSWindow *)sender
	{
		app_custom_event(self.windowNumber, WINDOW_EVENT_CLOSE, 0, 0);
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

	// NSResponder
	- (BOOL)performKeyEquivalent:(NSEvent *)event
	{
		// Prevents beeping on keystrokes
		return YES;
	}

	// NSWindowDelegate
	- (void)windowDidResignKey:(NSNotification *)notification
	{
	}

	- (void)windowDidBecomeKey:(NSNotification *)notification
	{
	}

	- (void)windowDidExpose:(NSNotification *)notification
	{
	}

	- (void)windowDidEnterFullScreen:(NSNotification *)notification
	{
	}

	- (void)windowDidResize:(NSNotification *)notification
	{
	}
@end


// NSApp

@interface App : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>
	@property MTY_AppFunc app_func;
	@property MTY_MsgFunc msg_func;
	@property void *opaque;
	@property void (*open_url)(const char *url, void *opaque);
	@property void *url_opaque;
	@property bool restart;
	@property bool should_minimize;
	@property bool relative;
	@property bool default_cursor;
	@property bool cursor_outside;
	@property uint32_t cb_seq;
	@property NSCursor *custom_cursor;
	@property NSCursor *cursor;
	@property void **windows;
@end

static void app_add_menu_item(NSMenu *menu, NSString *title, NSString *key, SEL sel)
{
	NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:sel keyEquivalent:key];
	[menu addItem:item];
}

static void app_add_menu_separator(NSMenu *menu)
{
	[menu addItem:[NSMenuItem separatorItem]];
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

	- (void)appClose:(id)sender
	{
		for (uint8_t x = 0; x < MTY_WINDOW_MAX; x++) {
			Window *window = (__bridge Window *) self.windows[x];
			if (window)
				app_custom_event(window.windowNumber, WINDOW_EVENT_CLOSE, 0, 0);
		}
	}

	- (void)appRestart:(id)sender
	{
		_restart = true;
	}

	- (void)appMinimize:(id)sender
	{
		_should_minimize = true;
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

		app_add_menu_item(menu, @"Quit", @"q", @selector(appClose:));
		app_add_menu_item(menu, @"Restart", @"", @selector(appRestart:));
		app_add_menu_separator(menu);
		app_add_menu_item(menu, @"Minimize", @"m", @selector(appMinimize:));
		app_add_menu_item(menu, @"Close", @"w", @selector(appClose:));

		[item setSubmenu:menu];
		[NSApp setMainMenu:menubar];
	}

	- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
	{
		[NSApp unhide:self];

		return NO;
	}

	- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
	{
		NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];

		if (url && _open_url)
			_open_url([url UTF8String], _url_opaque);
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


// Static helpers

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

static bool app_get_mty_window_by_number(App *ctx, NSInteger number, Window **out)
{
	for (int8_t x = 0; x < MTY_WINDOW_MAX; x++) {
		Window *window = (__bridge Window *) ctx.windows[x];

		if (window.windowNumber == number) {
			*out = window;
			return true;
		}
	}

	return false;
}


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
	App *ctx = (__bridge App *) app;

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	ctx.cb_seq = [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:[NSString stringWithUTF8String:text] forType:NSPasteboardTypeString];
}


// Cursor

static void app_apply_cursor(App *ctx)
{
	NSCursor *arrow = [NSCursor arrowCursor];
	NSCursor *new = ctx.default_cursor || ctx.cursor_outside ? arrow :
		ctx.custom_cursor ? ctx.custom_cursor : arrow;

	if (ctx.cursor)
		[ctx.cursor pop];

	ctx.cursor = new;
	[ctx.cursor push];
}

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
}


// Event processing

static Window *app_get_event_window(App *ctx, NSEvent *event)
{
	Window *window = nil;

	// First try to find the window based on what's reported by the event
	if (event.window)
		if (app_get_mty_window_by_number(ctx, event.window.windowNumber, &window))
			return window;

	// Try to use the current mouse coordinates to find the window
	NSInteger num = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0];
	if (app_get_mty_window_by_number(ctx, num, &window))
		return window;

	return nil;
}

static bool app_next_event(App *ctx)
{
	NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
	if (!event)
		return false;

	Window *window = app_get_event_window(ctx, event);

	MTY_Msg wmsg = {0};
	wmsg.window = window ? window.window : 0;

	switch (event.type) {
		case NSEventTypeKeyDown: {
			const char *text = [event.characters UTF8String];

			// Make sure visible ASCII
			if (text && text[0] && text[0] >= 0x20 && text[0] != 0x7F) {
				wmsg.type = MTY_WINDOW_MSG_TEXT;

				snprintf(wmsg.text, 8, "%s", text);
				ctx.msg_func(&wmsg, ctx.opaque);
				wmsg.type = MTY_WINDOW_MSG_NONE;
			}
		}

		case NSEventTypeKeyUp:
		case NSEventTypeFlagsChanged: {
			wmsg.keyboard.scancode = keycode_to_scancode(event.keyCode);
			wmsg.keyboard.mod = 0; // TODO

			if (wmsg.keyboard.scancode != MTY_SCANCODE_NONE) {
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
			wmsg.mouseWheel.x = lrint(event.deltaX) * 120;
			wmsg.mouseWheel.y = lrint(event.deltaY) * 120;
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
				case 0: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT; break;
				case 1: wmsg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT; break;
			}
			break;
		case NSEventTypeLeftMouseDragged:
		case NSEventTypeRightMouseDragged:
		case NSEventTypeOtherMouseDragged:
		case NSEventTypeMouseMoved: {
			if (ctx.relative) {
				wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
				wmsg.mouseMotion.relative = true;
				wmsg.mouseMotion.x = event.deltaX;
				wmsg.mouseMotion.y = event.deltaY;

			} else {
				if (window) {
					NSPoint p = [window mouseLocationOutsideOfEventStream];
					NSSize size = window.contentView.frame.size;
					int32_t scale = lrint(window.screen.backingScaleFactor);
					int32_t x = lrint(p.x) * scale;
					int32_t y = lrint(size.height - p.y) * scale;

					if (x >= 0 && y >= 0 && x <= size.width && y <= size.height) {
						wmsg.type = MTY_WINDOW_MSG_MOUSE_MOTION;
						wmsg.mouseMotion.relative = false;
						wmsg.mouseMotion.x = x;
						wmsg.mouseMotion.y = y;
					}
				}
			}
			break;
		}
		case NSEventTypeApplicationDefined:
			switch (event.subtype) {
				case WINDOW_EVENT_CLOSE:
					wmsg.type = MTY_WINDOW_MSG_CLOSE;
					break;
				case WINDOW_EVENT_MOUSE_ENTERED:
				case WINDOW_EVENT_MOUSE_EXITED:
					ctx.cursor_outside = event.subtype == WINDOW_EVENT_MOUSE_EXITED;
					app_apply_cursor(ctx);
					break;
			}
			break;
	}

	if (wmsg.type != MTY_WINDOW_MSG_NONE)
		ctx.msg_func(&wmsg, ctx.opaque);

	[NSApp sendEvent:event];

	return true;
}


// App

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	App *ctx = [App new];
	ctx.app_func = appFunc;
	ctx.msg_func = msgFunc;
	ctx.opaque = opaque;

	ctx.windows = MTY_Alloc(MTY_WINDOW_MAX, sizeof(void *));

	[NSApplication sharedApplication];
	[NSApp setDelegate:ctx];
	[NSApp finishLaunching];

	return (MTY_App *) CFBridgingRetain(ctx);
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	App *ctx = (App *) CFBridgingRelease(*app);
	for (int8_t x = 0; x < MTY_WINDOW_MAX; x++)
		MTY_WindowDestroy(*app, x);

	MTY_Free(ctx.windows);
	ctx = nil;

	*app = NULL;
}

void MTY_AppRun(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	do {
		while (app_next_event(ctx));

	} while (ctx.app_func(ctx.opaque));
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
	// IOPMAssertionCreateWithDescription
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	// TODO
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
	// TODO
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	App *ctx = (__bridge App *) app;

	// NOTE: Argument is unused in CGDisplayXXXCursor functions

	if (relative && !ctx.relative) {
		ctx.relative = true;
		CGAssociateMouseAndMouseCursorPosition(NO);
		CGDisplayHideCursor(kCGDirectMainDisplay);

	} else if (!relative && ctx.relative) {
		ctx.relative = false;
		CGAssociateMouseAndMouseCursorPosition(YES);
		CGDisplayShowCursor(kCGDirectMainDisplay);
	}
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	App *ctx = (__bridge App *) app;

	return ctx.relative;
}

bool MTY_AppIsActive(MTY_App *app)
{
	bool r = false;

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		r = r || MTY_WindowIsActive(app, x);

	return r;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
	App *ctx = (__bridge App *) app;

	if (active) {
		[NSApp unhide:ctx];
		[NSApp activateIgnoringOtherApps:YES];

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

	App *app_ctx = nil;
	Window *ctx = nil;
	View *content = nil;

	window = app_find_open_window(app);
	if (window == -1) {
		r = false;
		MTY_Log("Maximum windows (MTY_WINDOW_MAX) of %u reached", MTY_WINDOW_MAX);
		goto except;
	}

	NSRect rect = NSMakeRect(0, 0, desc->width, desc->height);
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
		NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	ctx = [[Window alloc] initWithContentRect:rect styleMask:style backing:NSBackingStoreBuffered defer:NO];
	ctx.window = window;

	[ctx setDelegate:ctx];
	[ctx setAcceptsMouseMovedEvents:YES];

	content = [[View alloc] initWithFrame:[ctx contentRectForFrameRect:ctx.frame]];
	[ctx setContentView:content];

	app_ctx = (__bridge App *) app;
	app_ctx.windows[window] = (void *) CFBridgingRetain(ctx);

	ctx.title = [NSString stringWithUTF8String:title];
	[ctx center];

	MTY_WindowActivate(app, window, true);

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

	App *app_ctx = (__bridge App *) app;
	ctx = (Window *) CFBridgingRelease(app_ctx.windows[window]);

	[ctx close];
	ctx = nil;

	app_ctx.windows[window] = NULL;
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

	CGFloat title_bar_h = ctx.frame.size.height - ctx.contentView.frame.size.height;
	CGFloat window_bottom = ctx.screen.frame.origin.y + ctx.frame.origin.y + ctx.frame.size.height;

	NSPoint pscreen = {0};
	CGFloat scale = ctx.screen.backingScaleFactor;
	pscreen.x = ctx.screen.frame.origin.x + ctx.frame.origin.x + (CGFloat) x / scale;
	pscreen.y = ctx.screen.frame.size.height - window_bottom + title_bar_h + (CGFloat) y / scale;

	CGWarpMouseCursorPosition(pscreen);
	CGAssociateMouseAndMouseCursorPosition(YES);
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
