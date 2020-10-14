// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <EGL/egl.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <android/log.h>


static struct window_gfx {
	MTY_Mutex *mutex;
	ANativeWindow *window;
	bool ready;
	bool init;

	int32_t width;
	int32_t height;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
} GFX;

JNIEXPORT void JNICALL Java_group_matoya_merton_MainSurface_mty_1set_1surface(JNIEnv *env, jobject instance, jobject surface)
{
	MTY_MutexLock(GFX.mutex);

	GFX.window = ANativeWindow_fromSurface(env, surface);

	if (GFX.window)
		GFX.ready = true;

	MTY_MutexUnlock(GFX.mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_merton_MainSurface_mty_1surface_1dims(JNIEnv *env, jobject instance, jint w, jint h)
{
	MTY_MutexLock(GFX.mutex);

	GFX.width = w;
	GFX.height = h;

	MTY_MutexUnlock(GFX.mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_merton_MainSurface_mty_1unset_1surface(JNIEnv *env, jobject instance)
{
	MTY_MutexLock(GFX.mutex);

	if (GFX.surface)
		eglDestroySurface(GFX.display, GFX.surface);

	if (GFX.window)
		ANativeWindow_release(GFX.window);

	GFX.surface = 0;
	GFX.window = NULL;

	GFX.ready = false;
	GFX.init = false;

	MTY_MutexUnlock(GFX.mutex);
}


int main(int argc, char **argv);

JNIEXPORT void JNICALL Java_group_matoya_merton_MainActivity_mty_1global_1init(JNIEnv *env, jobject instance)
{
	GFX.mutex = MTY_MutexCreate();
}

JNIEXPORT void JNICALL Java_group_matoya_merton_AppThread_mty_1start(JNIEnv *env, jobject instance, jstring jname)
{
	const char *cname = (*env)->GetStringUTFChars(env, jname, 0);
	char *name = MTY_Strdup(cname);

	(*env)->ReleaseStringUTFChars(env, jname, cname);

	// __android_log_print(ANDROID_LOG_INFO, "MTY", "START");

	main(1, &name);

	MTY_Free(name);
}


struct MTY_Window {
	MTY_MsgFunc msg_func;
	const void *opaque;

	uint32_t fb0;
	MTY_GFX api;
	MTY_Renderer *renderer;
};

static void window_destroy_members(MTY_Window *ctx)
{
	MTY_RendererDestroy(&ctx->renderer);

	if (GFX.display) {
		if (GFX.context) {
			eglMakeCurrent(GFX.display, 0, 0, 0);
			eglDestroyContext(GFX.display, GFX.context);
		}

		if (GFX.surface)
			eglDestroySurface(GFX.display, GFX.surface);

		eglTerminate(GFX.display);
	}

	GFX.display = 0;
	GFX.surface = 0;
	GFX.context = 0;
}

static bool window_check(MTY_Window *ctx)
{
	bool r = true;

	if (!GFX.ready)
		return false;

	if (GFX.ready && GFX.init)
		goto except;

	window_destroy_members(ctx);

	GFX.display = eglGetDisplay(0);
	eglInitialize(GFX.display, NULL, NULL);

	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLint n;
	EGLConfig config;
	eglChooseConfig(GFX.display, attribs, &config, 1, &n);

	GFX.surface = eglCreateWindowSurface(GFX.display, config, GFX.window, NULL);

	const EGLint attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	GFX.context = eglCreateContext(GFX.display, config, 0, attrib);

	eglMakeCurrent(GFX.display, GFX.surface, GFX.surface, GFX.context);

	ctx->renderer = MTY_RendererCreate();

	GFX.init = true;

	except:

	if (!r)
		window_destroy_members(ctx);

	return r;
}

MTY_Window *MTY_WindowCreate(const char *title, MTY_MsgFunc msg_func, const void *opaque,
	uint32_t width, uint32_t height, bool fullscreen)
{
	MTY_Window *ctx = MTY_Alloc(1, sizeof(MTY_Window));
	ctx->msg_func = msg_func;
	ctx->opaque = opaque;
	ctx->api = MTY_GFX_GL;

	return ctx;
}

void MTY_AppRun(MTY_AppFunc func, const void *opaque)
{
	while (func((void *) opaque));
}

void MTY_WindowSetTitle(MTY_Window *ctx, const char *title, const char *subtitle)
{
}

bool MTY_WindowGetSize(MTY_Window *ctx, uint32_t *width, uint32_t *height)
{
	*width = GFX.width;
	*height = GFX.height;

	return true;
}

void MTY_WindowPoll(MTY_Window *ctx)
{
}

bool MTY_WindowIsForeground(MTY_Window *ctx)
{
	return true;
}

void MTY_WindowSetRelativeMouse(MTY_Window *ctx, bool relative)
{
}

uint32_t MTY_WindowGetRefreshRate(MTY_Window *ctx)
{
	return 60;
}

float MTY_WindowGetDPIScale(MTY_Window *ctx)
{
	return 2.0f;
}

void MTY_WindowSetFullscreen(MTY_Window *ctx)
{
}

void MTY_WindowSetSize(MTY_Window *ctx, uint32_t width, uint32_t height)
{
}

bool MTY_WindowIsFullscreen(MTY_Window *ctx)
{
	return false;
}

void MTY_WindowPresent(MTY_Window *ctx, uint32_t num_frames)
{
	MTY_MutexLock(GFX.mutex);

	if (window_check(ctx))
		eglSwapBuffers(GFX.display, GFX.surface);

	MTY_MutexUnlock(GFX.mutex);
}

MTY_Device *MTY_WindowGetDevice(MTY_Window *ctx)
{
	return NULL;
}

MTY_Context *MTY_WindowGetContext(MTY_Window *ctx)
{
	return NULL;
}

MTY_Texture *MTY_WindowGetBackBuffer(MTY_Window *ctx)
{
	return (MTY_Texture *) &ctx->fb0;
}

void MTY_WindowDrawQuad(MTY_Window *ctx, const void *image, const MTY_RenderDesc *desc)
{
	MTY_MutexLock(GFX.mutex);

	if (GFX.init && GFX.ready) {
		MTY_Texture *buffer = MTY_WindowGetBackBuffer(ctx);
		if (buffer) {
			MTY_RenderDesc mutated = *desc;
			mutated.viewWidth = GFX.width;
			mutated.viewHeight = GFX.height;

			MTY_Device *device = MTY_WindowGetDevice(ctx);
			MTY_Context *context = MTY_WindowGetContext(ctx);

			MTY_RendererDrawQuad(ctx->renderer, ctx->api, device, context, image, &mutated, buffer);
		}
	}

	MTY_MutexUnlock(GFX.mutex);
}

void MTY_WindowDrawUI(MTY_Window *ctx, const MTY_DrawData *dd)
{
	MTY_MutexLock(GFX.mutex);

	if (GFX.init && GFX.ready) {
		MTY_Texture *buffer = MTY_WindowGetBackBuffer(ctx);
		if (buffer) {
			MTY_Device *device = MTY_WindowGetDevice(ctx);
			MTY_Context *context = MTY_WindowGetContext(ctx);

			MTY_RendererDrawUI(ctx->renderer, ctx->api, device, context, dd, buffer);
		}
	}

	MTY_MutexUnlock(GFX.mutex);
}

void MTY_WindowSetUIFont(MTY_Window *ctx, const void *font, uint32_t width, uint32_t height)
{
	MTY_MutexLock(GFX.mutex);

	if (GFX.init && GFX.ready) {
		MTY_Device *device = MTY_WindowGetDevice(ctx);
		MTY_Context *context = MTY_WindowGetContext(ctx);

		MTY_RendererSetUIFont(ctx->renderer, ctx->api, device, context, font, width, height);
	}

	MTY_MutexUnlock(GFX.mutex);
}

void *MTY_WindowGetUIFontResource(MTY_Window *ctx)
{
	void *texture = NULL;

	MTY_MutexLock(GFX.mutex);

	if (GFX.init && GFX.ready)
		texture = MTY_RendererGetUIFontResource(ctx->renderer);

	MTY_MutexUnlock(GFX.mutex);

	return texture;
}

bool MTY_WindowGFXSetCurrent(MTY_Window *ctx)
{
	bool r = false;

	MTY_MutexLock(GFX.mutex);

	if (GFX.init && GFX.ready)
		r = eglMakeCurrent(GFX.display, GFX.surface, GFX.surface, GFX.context);

	MTY_MutexUnlock(GFX.mutex);

	return r;
}

void MTY_WindowDestroy(MTY_Window **window)
{
	if (!window || !*window)
		return;

	MTY_Window *ctx = *window;

	MTY_MutexLock(GFX.mutex);

	window_destroy_members(ctx);

	MTY_MutexUnlock(GFX.mutex);

	MTY_Free(ctx);
	*window = NULL;
}
