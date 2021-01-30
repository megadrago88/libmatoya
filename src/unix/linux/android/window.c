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
uint32_t gfx_width(void);
uint32_t gfx_height(void);
uint32_t gfx_should_present(void);


// JNI

int main(int argc, char **argv);

static MTY_Queue *APP_EVENTS;
static MTY_Hash *APP_CTRLRS;
static MTY_Mutex *APP_CTRLR_MUTEX;
static int32_t APP_DOUBLE_TAP;
static MTY_Input APP_INPUT = MTY_INPUT_TOUCHSCREEN;
static MTY_MouseButton APP_LONG_BUTTON;
static JavaVM *APP_JVM;
static jobject APP_MTY_OBJ;
static bool APP_CHECK_SCROLLER;
static bool LOG_THREAD;
static bool APP_DETACH;

static const MTY_Controller APP_ZEROED_CTRLR = {
	.id = 0,
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

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1start(JNIEnv *env, jobject obj,
	jstring jname)
{
	APP_EVENTS = MTY_QueueCreate(500, sizeof(MTY_Msg));
	APP_CTRLRS = MTY_HashCreate(0);
	APP_CTRLR_MUTEX = MTY_MutexCreate();

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
	MTY_MutexDestroy(&APP_CTRLR_MUTEX);
	MTY_HashDestroy(&APP_CTRLRS, MTY_Free);
	MTY_QueueDestroy(&APP_EVENTS);
}


// JNI Helpers

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


// JNI keyboard events

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


// JNI touch events -- These are subject to "MTY_AppSetInputMode"

static void app_cancel_long_button(void)
{
	APP_DOUBLE_TAP = false;

	if (APP_LONG_BUTTON != MTY_MOUSE_BUTTON_NONE) {
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_MOUSE_BUTTON;
		msg.mouseButton.pressed = false;
		msg.mouseButton.button = APP_LONG_BUTTON;
		app_push_msg(&msg);

		APP_LONG_BUTTON = MTY_MOUSE_BUTTON_NONE;
	}
}

static void app_touch_mouse_button(int32_t x, int32_t y, MTY_MouseButton button)
{
	app_cancel_long_button();

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = lrint(x);
	msg.mouseMotion.y = lrint(y);
	msg.mouseMotion.click = true;
	app_push_msg(&msg);

	msg.type = MTY_MSG_MOUSE_BUTTON;
	msg.mouseButton.pressed = true;
	msg.mouseButton.button = button;
	app_push_msg(&msg);

	msg.mouseButton.pressed = false;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1unhandled_1touch(JNIEnv *env, jobject obj,
	jint action, jfloat x, jfloat y, jint fingers)
{
	APP_DETACH = false;

	// Any time fingers come off of the screen we cancel the LONG BUTTON
	if (action == AMOTION_EVENT_ACTION_UP)
		app_cancel_long_button();

	if (APP_INPUT != MTY_INPUT_TOUCHSCREEN) {
		switch (action) {
			case AMOTION_EVENT_ACTION_MOVE:
				// While a long press is in effect, move events get reported here
				// They do NOT come through in the onScroll gesture handler
				if (APP_LONG_BUTTON != MTY_MOUSE_BUTTON_NONE) {
					MTY_Msg msg = {0};
					msg.type = MTY_MSG_MOUSE_MOTION;
					msg.mouseMotion.x = lrint(x);
					msg.mouseMotion.y = lrint(y);
					app_push_msg(&msg);
				}
				break;
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				// Taps with two fingers need to be handled manually
				// They are not detected by the gesture recognizer
				APP_DOUBLE_TAP = fingers > 1;
				break;
			case AMOTION_EVENT_ACTION_POINTER_UP:
				if (APP_DOUBLE_TAP)
					app_touch_mouse_button(x, y, MTY_MOUSE_BUTTON_RIGHT);

				APP_DOUBLE_TAP = false;
				break;
		}
	}
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1single_1tap_1up(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	APP_DETACH = false;

	app_touch_mouse_button(x, y, MTY_MOUSE_BUTTON_LEFT);
}

JNIEXPORT jboolean JNICALL Java_group_matoya_lib_MTYSurface_app_1long_1press(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	APP_DETACH = false;

	app_cancel_long_button();

	// Long press always begins by moving to the event location
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.mouseMotion.x = lrint(x);
	msg.mouseMotion.y = lrint(y);
	msg.mouseMotion.click = true;
	app_push_msg(&msg);

	// Long press in touchscreen mode is a simple right click
	if (APP_INPUT == MTY_INPUT_TOUCHSCREEN) {
		msg.type = MTY_MSG_MOUSE_BUTTON;
		msg.mouseButton.pressed = true;
		msg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;
		app_push_msg(&msg);

		msg.mouseButton.pressed = false;
		app_push_msg(&msg);

	// In trackpad mode, begin a mouse down and set the LONG_BUTTON state
	} else {
		APP_LONG_BUTTON = MTY_MOUSE_BUTTON_LEFT;

		msg.type = MTY_MSG_MOUSE_BUTTON;
		msg.mouseButton.pressed = true;
		msg.mouseButton.button = APP_LONG_BUTTON;
		app_push_msg(&msg);

		return true;
	}

	return false;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1scroll(JNIEnv *env, jobject obj,
	jfloat abs_x, jfloat abs_y, jfloat x, jfloat y, jint fingers)
{
	APP_DETACH = false;

	app_cancel_long_button();

	// Single finger scrolling in touchscreen mode OR two finger scrolling in
	// trackpad mode moves to the touch location and produces a wheel event
	if (APP_INPUT == MTY_INPUT_TOUCHSCREEN ||
		(APP_LONG_BUTTON == MTY_MOUSE_BUTTON_NONE && fingers > 1))
	{
		MTY_Msg msg = {0};

		// Negative init values mean "don't move the cursor"
		if (abs_x > 0.0f || abs_y > 0.0f) {
			msg.type = MTY_MSG_MOUSE_MOTION;
			msg.mouseMotion.x = lrint(abs_x);
			msg.mouseMotion.y = lrint(abs_y);
			app_push_msg(&msg);
		}

		msg.type = MTY_MSG_MOUSE_WHEEL;
		msg.mouseWheel.pixels = true;
		msg.mouseWheel.y = -lrint(y);
		msg.mouseWheel.x = 0;
		app_push_msg(&msg);

	// While single finger scrolling in trackpad mode, convert to mouse motion
	} else {
		MTY_Msg msg = {0};
		msg.type = MTY_MSG_MOUSE_MOTION;
		msg.mouseMotion.x = lrint(abs_x);
		msg.mouseMotion.y = lrint(abs_y);
		app_push_msg(&msg);
	}
}


// JNI mouse events

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1mouse_1motion(JNIEnv *env, jobject obj,
	jboolean relative, jfloat x, jfloat y)
{
	APP_DETACH = true;

	app_cancel_long_button();

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
	APP_DETACH = true;

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_WHEEL;
	msg.mouseWheel.x = x > 0.0f ? 120 : x < 0.0f ? -120 : 0;
	msg.mouseWheel.y = y > 0.0f ? 120 : y < 0.0f ? -120 : 0;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1mouse_1button(JNIEnv *env, jobject obj,
	jboolean pressed, jint button, jfloat x, jfloat y)
{
	APP_DETACH = true;

	app_cancel_long_button();

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


// JNI controller events

static MTY_Controller *app_get_controller(int32_t deviceId)
{
	MTY_Controller *c = MTY_HashGetInt(APP_CTRLRS, deviceId);
	if (!c) {
		c = MTY_Alloc(1, sizeof(MTY_Controller));
		*c = APP_ZEROED_CTRLR;

		c->id = deviceId;
		MTY_HashSetInt(APP_CTRLRS, deviceId, c);

		MTY_Msg msg = {0};
		msg.type = MTY_MSG_CONNECT;
		msg.controller = *c;
		app_push_msg(&msg);
	}

	return c;
}

static void app_remove_controller(int32_t deviceId)
{
	MTY_Controller *c = MTY_HashGetInt(APP_CTRLRS, deviceId);
	if (c) {
		*c = APP_ZEROED_CTRLR;

		c->id = deviceId;

		MTY_Msg msg = {0};
		msg.type = MTY_MSG_DISCONNECT;
		msg.controller = *c;
		app_push_msg(&msg);

		MTY_Free(MTY_HashPopInt(APP_CTRLRS, deviceId));
	}
}

static void app_push_cmsg(const MTY_Controller *c)
{
	MTY_Msg msg = {0};
	msg.type = MTY_MSG_CONTROLLER;
	msg.controller = *c;
	app_push_msg(&msg);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1button(JNIEnv *env, jobject obj,
	jint deviceId, jboolean pressed, jint button)
{
	MTY_MutexLock(APP_CTRLR_MUTEX);

	MTY_Controller *c = app_get_controller(deviceId);

	switch (button) {
		case AKEYCODE_BUTTON_A: c->buttons[MTY_CBUTTON_A] = pressed; break;
		case AKEYCODE_BUTTON_B: c->buttons[MTY_CBUTTON_B] = pressed; break;
		case AKEYCODE_BUTTON_X: c->buttons[MTY_CBUTTON_X] = pressed; break;
		case AKEYCODE_BUTTON_Y: c->buttons[MTY_CBUTTON_Y] = pressed; break;
		case AKEYCODE_BUTTON_L1: c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_R1: c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_L2: c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_R2: c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_THUMBL: c->buttons[MTY_CBUTTON_LEFT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_THUMBR: c->buttons[MTY_CBUTTON_RIGHT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_START: c->buttons[MTY_CBUTTON_START] = pressed; break;
		case AKEYCODE_BUTTON_SELECT: c->buttons[MTY_CBUTTON_BACK] = pressed; break;
		case AKEYCODE_BUTTON_MODE: c->buttons[MTY_CBUTTON_GUIDE] = pressed; break;
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

		c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
			(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	}

	app_push_cmsg(c);

	MTY_MutexUnlock(APP_CTRLR_MUTEX);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1axis(JNIEnv *env, jobject obj,
	jint deviceId, jfloat hatX, jfloat hatY, jfloat lX, jfloat lY, jfloat rX, jfloat rY, jfloat lT, jfloat rT)
{
	MTY_MutexLock(APP_CTRLR_MUTEX);

	MTY_Controller *c = app_get_controller(deviceId);

	c->values[MTY_CVALUE_THUMB_LX].data = lrint(lX * (float) (lX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_LY].data = lrint(-lY * (float) (-lY < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_RX].data = lrint(rX * (float) (rX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_RY].data = lrint(-rY * (float) (-rY < 0.0f ? abs(INT16_MIN) : INT16_MAX));

	c->values[MTY_CVALUE_TRIGGER_L].data = lrint(lT * (float) UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_R].data = lrint(rT * (float) UINT8_MAX);

	bool up = hatY == -1.0f;
	bool down = hatY == 1.0f;
	bool left = hatX == -1.0f;
	bool right = hatX == 1.0f;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;

	app_push_cmsg(c);

	MTY_MutexUnlock(APP_CTRLR_MUTEX);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_app_1unplug(JNIEnv *env, jobject obj,
	jint deviceId)
{
	MTY_MutexLock(APP_CTRLR_MUTEX);

	app_remove_controller(deviceId);

	MTY_MutexUnlock(APP_CTRLR_MUTEX);
}


// App

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
	jmethodID mid = (*env)->GetMethodID(env, cls, "getDisplayDensity", "()F");

	jfloat density = (*env)->CallFloatMethod(env, APP_MTY_OBJ, mid);

	return density / 185.0f;
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
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	// When touch events are receved, don't respect detach

	return APP_DETACH ? app->detach : MTY_DETACH_NONE;
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

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
	APP_INPUT = mode;
}


// Window

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return 0;
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
	*width = gfx_width();
	*height = gfx_height();

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

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return gfx_is_ready();
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return gfx_is_ready();
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return MTY_WindowGetScreenSize(app, window, width, height);
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

bool MTY_WindowGFXShouldPresent(MTY_App *app, MTY_Window window)
{
	return gfx_should_present();
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

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

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
