// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "scancode.h"

@interface App : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>
	@property(strong) NSTimer *timer;
	@property MTY_AppFunc app_func;
	@property MTY_MsgFunc msg_func;
	@property void *opaque;
	@property void (*open_url)(const char *url, void *opaque);
	@property void *url_opaque;
	@property bool cont;
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

@interface Window : NSWindow <NSWindowDelegate>
	@property(strong) App *app;
	@property MTY_Window window;
	@property MTY_GFX api;
	@property struct gfx_ctx *gfx_ctx;
@end

@interface View : NSView
	@property NSTrackingArea *area;
@end


// NSView

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


// NSWindow

static MTY_Msg window_msg(Window *window, MTY_MsgType type)
{
	MTY_Msg msg = {0};
	msg.type = type;
	msg.window = window.window;

	return msg;
}

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

static bool window_event_in_view(Window *window, NSPoint *p)
{
	*p = [window mouseLocationOutsideOfEventStream];
	NSSize size = window.contentView.frame.size;

	p->y = size.height - p->y;

	return p->x >= 0 && p->y >= 0 && p->x < size.width && p->y < size.height;
}

static void window_mouse_event(Window *window, MTY_MouseButton button, bool pressed)
{
	NSPoint p = {0};

	if (window_event_in_view(window, &p)) {
		MTY_Msg msg = window_msg(window, MTY_WINDOW_MSG_MOUSE_BUTTON);
		msg.mouseButton.pressed = pressed;
		msg.mouseButton.button = button;

		window.app.msg_func(&msg, window.app.opaque);
	}
}

static void window_motion_event(Window *window, NSEvent *event)
{
	MTY_Msg msg = window_msg(window, MTY_WINDOW_MSG_MOUSE_MOTION);

	if (window.app.relative) {
		msg.mouseMotion.relative = true;
		msg.mouseMotion.x = event.deltaX;
		msg.mouseMotion.y = event.deltaY;

		window.app.msg_func(&msg, window.app.opaque);

	} else {
		NSPoint p = {0};

		if (window_event_in_view(window, &p)) {
			CGFloat scale = window.screen.backingScaleFactor;
			msg.mouseMotion.relative = false;
			msg.mouseMotion.x = lrint(scale * p.x);
			msg.mouseMotion.y = lrint(scale * p.y);

			window.app.msg_func(&msg, window.app.opaque);
		}
	}
}

static void window_keyboard_event(Window *window, int16_t key_code, NSEventModifierFlags flags, bool pressed)
{
	MTY_Msg msg = window_msg(window, MTY_WINDOW_MSG_KEYBOARD);
	msg.keyboard.scancode = keycode_to_scancode(key_code);
	msg.keyboard.mod = modifier_flags_to_keymod(flags);
	msg.keyboard.pressed = pressed;

	if (msg.keyboard.scancode != MTY_SCANCODE_NONE)
		window.app.msg_func(&msg, window.app.opaque);
}

@implementation Window : NSWindow
	- (BOOL)windowShouldClose:(NSWindow *)sender
	{
		MTY_Msg msg = window_msg(self, MTY_WINDOW_MSG_CLOSE);
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

	// NSResponder
	- (BOOL)acceptsFirstResponder
	{
		return YES;
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
			MTY_Msg msg = window_msg(self, MTY_WINDOW_MSG_TEXT);
			snprintf(msg.text, 8, "%s", text);

			self.app.msg_func(&msg, self.app.opaque);
		}

		window_keyboard_event(self, event.keyCode, event.modifierFlags, true);
	}

	- (void)flagsChanged:(NSEvent *)event
	{
		MTY_Msg msg = window_msg(self, MTY_WINDOW_MSG_KEYBOARD);
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
		// TODO
	}

	- (void)otherMouseDown:(NSEvent *)event
	{
		// TODO
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
		MTY_Msg msg = window_msg(self, MTY_WINDOW_MSG_MOUSE_WHEEL);
		msg.mouseWheel.x = lrint(event.deltaX) * 120;
		msg.mouseWheel.y = lrint(event.deltaY) * 120;

		self.app.msg_func(&msg, self.app.opaque);
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
	}

	- (void)appRestart:(id)sender
	{
		_restart = true;
	}

	- (void)appMinimize:(id)sender
	{
		_should_minimize = true;
	}

	- (void)appFunc:(id)sender
	{
		self.cont = self.app_func(self.opaque);
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

		self.timer = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self
			selector:@selector(appFunc:) userInfo:nil repeats:YES];
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

	[ctx.timer invalidate];
	ctx.timer = nil;
	ctx = nil;

	*app = NULL;
}

void MTY_AppRun(MTY_App *app)
{
	[NSApp run];
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
	ctx.app = (__bridge App *) app;

	[ctx setDelegate:ctx];
	[ctx setAcceptsMouseMovedEvents:YES];

	content = [[View alloc] initWithFrame:[ctx contentRectForFrameRect:ctx.frame]];
	[ctx setContentView:content];

	ctx.app.windows[window] = (void *) CFBridgingRetain(ctx);

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
