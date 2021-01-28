// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "keymap.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <jni.h>
#include <android/log.h>
#include <android/input.h>

struct MTY_App {
	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Detach detach;
	uint32_t timeout;
	float scale;
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
static bool APP_CHECK_SCROLLER;
static bool LOG_THREAD;

static bool APP_CTRLR_FIRST = true;
static MTY_Controller APP_CTRLR = {
	.id = 1,
	.pid = 0xCDD,
	.vid = 0xCDD,
	.numButtons = 13,
	.numValues = 7,
	.driver = MTY_HID_DRIVER_DEFAULT,
	.values = {
		[MTY_CVALUE_THUMB_LX] = {
			.usage = 0x30,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_LY] = {
			.usage = 0x31,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_RX] = {
			.usage = 0x32,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_RY] = {
			.usage = 0x35,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_TRIGGER_L] = {
			.usage = 0x33,
			.min = 0,
			.max = UINT8_MAX,
		},
		[MTY_CVALUE_TRIGGER_R] = {
			.usage = 0x34,
			.min = 0,
			.max = UINT8_MAX,
		},
		[MTY_CVALUE_DPAD] = {
			.usage = 0x39,
			.data = 8,
			.min = 0,
			.max = 7,
		},
	},
};

static void *app_log_thread(void *opaque)
{
	// stdout & stderr redirection
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	int32_t pfd[2] = {0};
	pipe(pfd);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	while (LOG_THREAD) {
		char buf[512];
		ssize_t size = read(pfd[0], buf, 511);

		if (size <= 0)
			break;

		if (buf[size - 1] == '\n')
			size--;

		buf[size] = '\0';

		__android_log_write(ANDROID_LOG_DEBUG, "MTY", buf);
	}

	return NULL;
}

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

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1dims(JNIEnv *env, jobject obj,
	jint w, jint h)
{
	APP_WIDTH = w;
	APP_HEIGHT = h;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1start(JNIEnv *env, jobject obj,
	jstring jname)
{
	APP_EVENTS = MTY_QueueCreate(500, sizeof(MTY_Msg));

	LOG_THREAD = true;
	MTY_Thread *log_thread = MTY_ThreadCreate(app_log_thread, NULL);

	const char *cname = (*env)->GetStringUTFChars(env, jname, 0);
	char *name = MTY_Strdup(cname);
	(*env)->ReleaseStringUTFChars(env, jname, cname);

	APP_MTY_OBJ = (*env)->NewGlobalRef(env, obj);

	// Make current working directory the package's writable area
	chdir(MTY_Path("/data/data", name));

	char *argv[2] = {name, NULL};
	main(1, argv);

	LOG_THREAD = false;
	printf("\n");

	MTY_Free(name);
	MTY_ThreadDestroy(&log_thread);
	MTY_QueueDestroy(&APP_EVENTS);
}


// JNI events

static void app_push_msg(MTY_Msg *msg)
{
	MTY_Msg *qmsg = MTY_QueueAcquireBuffer(APP_EVENTS);
	*qmsg = *msg;

	MTY_QueuePush(APP_EVENTS, sizeof(MTY_Msg));
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1check_1scroller(JNIEnv *env, jobject obj,
	jboolean check)
{
	APP_CHECK_SCROLLER = check;
}

JNIEXPORT jboolean JNICALL Java_group_matoya_lib_MTYSurface_app_1key(JNIEnv *env, jobject obj,
	jboolean pressed, jint code, jint itext, jint mods)
{
	// If trap is false, android will handle the key
	bool trap = false;

	if (pressed && itext != 0) {
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_TEXT;
		memcpy(msg.text, &itext, sizeof(jint));

		app_push_msg(&msg);
		trap = true;
	}

	if (pressed && code == AKEYCODE_BACK) {
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_BACK;
		app_push_msg(&msg);
		trap = true;
	}

	if (code < (jint) APP_KEYS_MAX) {
		MTY_Key key = APP_KEYS[code];
		if (key != MTY_KEY_NONE) {
			MTY_Msg msg = {0};
			msg.type = MTY_MSG_KEYBOARD;
			msg.keyboard.key = key;
			msg.keyboard.pressed = pressed;
			app_push_msg(&msg);
			trap = true;
		}
	}

	return trap;
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

	// Negative init values mean "don't move the cursor"
	if (init_x > 0.0f || init_y > 0.0f) {
		msg.type = MTY_MSG_MOUSE_MOTION;
		msg.mouseMotion.x = lrint(init_x);
		msg.mouseMotion.y = lrint(init_y);
		app_push_msg(&msg);
	}

	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.y = -lrint(y);
	msg.mouseWheel.x = 0;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1mouse_1motion(JNIEnv *env, jobject obj,
	jboolean relative, jfloat x, jfloat y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.relative = relative;
	msg.mouseMotion.x = lrint(x);
	msg.mouseMotion.y = lrint(y);
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1mouse_1wheel(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.x = x > 0.0f ? 120 : x < 0.0f ? -120 : 0;
	msg.mouseWheel.y = y > 0.0f ? 120 : y < 0.0f ? -120 : 0;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1mouse_1button(JNIEnv *env, jobject obj,
	jboolean pressed, jint button, jfloat x, jfloat y)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = pressed;

	switch (button) {
		case AMOTION_EVENT_BUTTON_PRIMARY:
			msg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT;
			break;
		case AMOTION_EVENT_BUTTON_SECONDARY:
			msg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;
			break;
		case AMOTION_EVENT_BUTTON_TERTIARY:
			msg.mouseButton.button = MTY_MOUSE_BUTTON_MIDDLE;
			break;
		case AMOTION_EVENT_BUTTON_BACK:
			msg.mouseButton.button = MTY_MOUSE_BUTTON_X1;
			break;
		case AMOTION_EVENT_BUTTON_FORWARD:
			msg.mouseButton.button = MTY_MOUSE_BUTTON_X2;
			break;
	}

	if (msg.mouseButton.button != MTY_MOUSE_BUTTON_NONE) {
		if (!MTY_AppGetRelativeMouse(NULL)) {
			MTY_Msg mv = {0};
			mv.type = MTY_MSG_MOUSE_MOTION;
			mv.mouseMotion.x = lrint(x);
			mv.mouseMotion.y = lrint(y);
			mv.mouseMotion.click = true;
			app_push_msg(&mv);
		}

		app_push_msg(&msg);
	}
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1button(JNIEnv *env, jobject obj,
	jboolean pressed, jint button)
{
	switch (button) {
		case AKEYCODE_BUTTON_A: APP_CTRLR.buttons[MTY_CBUTTON_A] = pressed; break;
		case AKEYCODE_BUTTON_B: APP_CTRLR.buttons[MTY_CBUTTON_B] = pressed; break;
		case AKEYCODE_BUTTON_X: APP_CTRLR.buttons[MTY_CBUTTON_X] = pressed; break;
		case AKEYCODE_BUTTON_Y: APP_CTRLR.buttons[MTY_CBUTTON_Y] = pressed; break;
		case AKEYCODE_BUTTON_L1: APP_CTRLR.buttons[MTY_CBUTTON_LEFT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_R1: APP_CTRLR.buttons[MTY_CBUTTON_RIGHT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_L2: APP_CTRLR.buttons[MTY_CBUTTON_LEFT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_R2: APP_CTRLR.buttons[MTY_CBUTTON_RIGHT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_THUMBL: APP_CTRLR.buttons[MTY_CBUTTON_LEFT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_THUMBR: APP_CTRLR.buttons[MTY_CBUTTON_RIGHT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_START: APP_CTRLR.buttons[MTY_CBUTTON_START] = pressed; break;
		case AKEYCODE_BUTTON_SELECT: APP_CTRLR.buttons[MTY_CBUTTON_BACK] = pressed; break;
		case AKEYCODE_BUTTON_MODE: APP_CTRLR.buttons[MTY_CBUTTON_GUIDE] = pressed; break;
	}

	if (button == AKEYCODE_DPAD_UP ||
		button == AKEYCODE_DPAD_RIGHT ||
		button == AKEYCODE_DPAD_DOWN ||
		button == AKEYCODE_DPAD_LEFT ||
		button == AKEYCODE_DPAD_CENTER ||
		button == AKEYCODE_DPAD_UP_LEFT ||
		button == AKEYCODE_DPAD_DOWN_LEFT ||
		button == AKEYCODE_DPAD_UP_RIGHT ||
		button == AKEYCODE_DPAD_DOWN_RIGHT)
	{
		bool up = button == AKEYCODE_DPAD_UP || button == AKEYCODE_DPAD_UP_LEFT || button == AKEYCODE_DPAD_UP_RIGHT;
		bool down = button == AKEYCODE_DPAD_DOWN || button == AKEYCODE_DPAD_DOWN_LEFT || button == AKEYCODE_DPAD_DOWN_RIGHT;
		bool left = button == AKEYCODE_DPAD_LEFT || button == AKEYCODE_DPAD_UP_LEFT || button == AKEYCODE_DPAD_DOWN_LEFT;
		bool right = button == AKEYCODE_DPAD_RIGHT || button == AKEYCODE_DPAD_UP_RIGHT || button == AKEYCODE_DPAD_DOWN_RIGHT;

		APP_CTRLR.values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
			(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	}

	MTY_Msg msg = {0};
	msg.controller = APP_CTRLR;

	if (APP_CTRLR_FIRST) {
		APP_CTRLR_FIRST = false;

		msg.type = MTY_MSG_CONNECT;
		app_push_msg(&msg);
	}

	msg.type = MTY_MSG_CONTROLLER;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1axis(JNIEnv *env, jobject obj,
	jfloat hatX, jfloat hatY, jfloat lX, jfloat lY, jfloat rX, jfloat rY, jfloat lT, jfloat rT)
{
	APP_CTRLR.values[MTY_CVALUE_THUMB_LX].data = lrint(lX * (float) (lX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	APP_CTRLR.values[MTY_CVALUE_THUMB_LY].data = lrint(-lY * (float) (-lY < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	APP_CTRLR.values[MTY_CVALUE_THUMB_RX].data = lrint(rX * (float) (rX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	APP_CTRLR.values[MTY_CVALUE_THUMB_RY].data = lrint(-rY * (float) (-rY < 0.0f ? abs(INT16_MIN) : INT16_MAX));

	APP_CTRLR.values[MTY_CVALUE_TRIGGER_L].data = lrint(lT * (float) UINT8_MAX);
	APP_CTRLR.values[MTY_CVALUE_TRIGGER_R].data = lrint(rT * (float) UINT8_MAX);

	bool up = hatY == -1.0f;
	bool down = hatY == 1.0f;
	bool left = hatX == -1.0f;
	bool right = hatX == 1.0f;

	APP_CTRLR.values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;

	MTY_Msg msg = {0};
	msg.controller = APP_CTRLR;

	if (APP_CTRLR_FIRST) {
		APP_CTRLR_FIRST = false;

		msg.type = MTY_MSG_CONNECT;
		app_push_msg(&msg);
	}

	msg.type = MTY_MSG_CONTROLLER;
	app_push_msg(&msg);
}


// App / window

char *MTY_AppGetClipboard(MTY_App *app)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "getClipboard", "()Ljava/lang/String;");

	jstring jtext = (*env)->CallObjectMethod(env, APP_MTY_OBJ, mid);
	if (jtext) {
		const char *ctext = (*env)->GetStringUTFChars(env, jtext, 0);
		char *text = MTY_Strdup(ctext);

		(*env)->ReleaseStringUTFChars(env, jtext, ctext);

		return text;
	}

	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "setClipboard", "(Ljava/lang/String;)V");

	jstring jtext = (*env)->NewStringUTF(env, text);

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, jtext);
}

static float app_get_scale(void)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "getDisplayDensity", "()I");

	jint density = (*env)->CallIntMethod(env, APP_MTY_OBJ, mid);

	return (float) density / 176.0f;
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;

	ctx->scale = app_get_scale();

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

static void app_check_scroller(void)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "checkScroller", "()V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid);
}

static bool app_check_focus(MTY_App *ctx, bool was_ready)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_FOCUS;
	msg.focus = gfx_is_ready();

	if ((!was_ready && msg.focus) || (was_ready && !msg.focus))
		ctx->msg_func(&msg, ctx->opaque);

	return msg.focus;
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true, was_ready = false; cont;) {
		for (MTY_Msg *msg; MTY_QueuePop(APP_EVENTS, 0, (void **) &msg, NULL);) {
			ctx->msg_func(msg, ctx->opaque);
			MTY_QueueReleaseBuffer(APP_EVENTS);
		}

		// Generate MTY_MSG_FOCUS events
		was_ready = app_check_focus(ctx, was_ready);

		// Generates scroll events after a fling has taken place
		// Prevent JNI calls if there's no fling in progress
		if (APP_CHECK_SCROLLER)
			app_check_scroller();

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
	return app->scale;
}

bool MTY_WindowGFXNewContext(MTY_App *app, MTY_Window window, bool reset)
{
	return gfx_was_reinit(reset);
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "setCursor", "([BFF)V");

	jbyteArray jimage = 0;

	if (image && size > 0) {
		jimage = (*env)->NewByteArray(env, size);
		(*env)->SetByteArrayRegion(env, jimage, 0, size, image);
	}

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, jimage, (jfloat) hotX, (jfloat) hotY);
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
bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return MTY_WindowGetScreenSize(app, window, width, height);
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "showCursor", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, show);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "useDefaultCursor", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, useDefault);
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "setRelativeMouse", "(Z)V");

	(*env)->CallVoidMethod(env, APP_MTY_OBJ, mid, relative);
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	JNIEnv *env = MTY_JNIEnv();

	jclass cls = (*env)->GetObjectClass(env, APP_MTY_OBJ);
	jmethodID mid = (*env)->GetMethodID(env, cls, "getRelativeMouse", "()Z");

	return (*env)->CallBooleanMethod(env, APP_MTY_OBJ, mid);
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	// TODO - This may need behavior dependent on device
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	// TODO - This may need behavior dependent on device
	return app->detach;
}

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	// TODO
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	// TODO
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
	// TODO
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

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
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
