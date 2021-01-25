// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <math.h>
#include <unistd.h>

#include <jni.h>
#include <android/log.h>

struct MTY_App {
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Detach detach;
	uint32_t timeout;
	void *opaque;


	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
};


// From GFX module

bool gfx_is_ready(void);
bool gfx_was_reinit(bool reset);


// JNI

int main(int argc, char **argv);

MTY_Queue *APP_EVENTS;
static JavaVM *APP_JVM;
static jobject APP_MTY_OBJ;
static uint32_t APP_WIDTH = 1920;
static uint32_t APP_HEIGHT = 1080;

void *MTY_JNIEnv(void)
{
	JNIEnv *env = NULL;

	if ((*APP_JVM)->GetEnv(APP_JVM, (void **) &env, JNI_VERSION_1_4) < 0)
		(*APP_JVM)->AttachCurrentThread(APP_JVM, &env, NULL);

	return env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	// This is safe to store globally at any point
	APP_JVM = vm;

	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1dims(JNIEnv *env, jobject instance, jint w, jint h)
{
	APP_WIDTH = w;
	APP_HEIGHT = h;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1start(JNIEnv *env, jobject instance, jstring jname)
{
	APP_EVENTS = MTY_QueueCreate(500, sizeof(MTY_Msg));

	const char *cname = (*env)->GetStringUTFChars(env, jname, 0);
	char *name = MTY_Strdup(cname);

	(*env)->ReleaseStringUTFChars(env, jname, cname);

	APP_MTY_OBJ = (*env)->NewGlobalRef(env, instance);

	// __android_log_print(ANDROID_LOG_INFO, "MTY", "START");

	chdir("/data/data/tv.parsec.client/");

	char *argv[2] = {name, NULL};
	main(1, argv);

	MTY_Free(name);
}


// JNI events

static void app_push_msg(MTY_Msg *msg)
{
	MTY_Msg *qmsg = MTY_QueueAcquireBuffer(APP_EVENTS);
	*qmsg = *msg;

	MTY_QueuePush(APP_EVENTS, sizeof(MTY_Msg));
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1key(JNIEnv *env, jobject obj,
	jboolean pressed, jint code, jint itext, jint mods)
{
	if (pressed && itext != 0) {
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_TEXT;
		memcpy(msg.text, &itext, sizeof(jint));

		app_push_msg(&msg);
	}
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1single_1tap_1up(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	// Currently treat single tap up gesture as a motion event and a full left click

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = lrint(x);
	msg.mouseMotion.y = lrint(y);
	app_push_msg(&msg);

	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = true;
	msg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT;
	app_push_msg(&msg);

	msg.mouseButton.pressed = false;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1long_1press(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	// Currently treat long press gesture as a motion event and a full right click

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = lrint(x);
	msg.mouseMotion.y = lrint(y);
	app_push_msg(&msg);

	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = true;
	msg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;
	app_push_msg(&msg);

	msg.mouseButton.pressed = false;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1scroll(JNIEnv *env, jobject obj,
	jfloat init_x, jfloat init_y, jfloat x, jfloat y)
{
	// Move the cursor to the initial location of the scroll, then send mouse wheel event

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = lrint(init_x);
	msg.mouseMotion.y = lrint(init_y);
	app_push_msg(&msg);

	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.y = -lrint(y);
	msg.mouseWheel.x = 0;
	app_push_msg(&msg);
}


// App / window

char *MTY_AppGetClipboard(MTY_App *app)
{
	// TODO
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	// TODO
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true; cont;) {
		for (MTY_Msg *msg; MTY_QueuePop(APP_EVENTS, 0, (void **) &msg, NULL);) {
			ctx->msg_func(msg, ctx->opaque);
			MTY_QueueReleaseBuffer(APP_EVENTS);
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

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "enableScreenSaver", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, enable);
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return gfx_is_ready();
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	// TODO
}

void MTY_AppSetOnscreenKeyboard(MTY_App *app, bool enable)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "showKeyboard", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, enable);
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "setOrientation", "(I)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, orientation);
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "enableFullscreen", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, fullscreen);
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "isFullscreen", "()Z");

	return (*env)->CallBooleanMethod(env, APP_MTY_OBJ, mid);
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	*width = APP_WIDTH;
	*height = APP_HEIGHT;

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	// TODO
	return 2.5f;
}

bool MTY_WindowGFXNewContext(MTY_App *app, MTY_Window window, bool reset)
{
	return gfx_was_reinit(reset);
}


// Window Private

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	app->api = api;
	app->gfx_ctx = gfx_ctx;
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	if (gfx_ctx)
		*gfx_ctx = app->gfx_ctx;

	return app->api;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	return (void *) (uintptr_t) 0xCDD;
}


// Unimplemented / stubs

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return false;
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return gfx_is_ready();
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return gfx_is_ready();
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return 0;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return MTY_WindowGetScreenSize(app, window, width, height);
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

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

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}
