// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_gl_)

#include <string.h>

#include <EGL/egl.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "gfx/gl-dl.h"

static struct gfx_gl_ctx {
	ANativeWindow *window;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	MTY_Mutex *mutex;
	MTY_Renderer *renderer;
	bool ready;
	bool init;
	bool reinit;
	uint32_t width;
	uint32_t height;
	uint32_t fb0;
} CTX;


// JNI

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_gfx_1global_1init(JNIEnv *env, jobject instance)
{
	CTX.mutex = MTY_MutexCreate();
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_gfx_1dims(JNIEnv *env, jobject instance, jint w, jint h)
{
	CTX.width = w;
	CTX.height = h;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_gfx_1set_1surface(JNIEnv *env, jobject instance, jobject surface)
{
	MTY_MutexLock(CTX.mutex);

	CTX.window = ANativeWindow_fromSurface(env, surface);

	if (CTX.window) {
		CTX.ready = true;
		CTX.reinit = true;
	}

	MTY_MutexUnlock(CTX.mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTYSurface_gfx_1unset_1surface(JNIEnv *env, jobject instance)
{
	MTY_MutexLock(CTX.mutex);

	if (CTX.surface)
		eglDestroySurface(CTX.display, CTX.surface);

	if (CTX.window)
		ANativeWindow_release(CTX.window);

	CTX.surface = 0;
	CTX.window = NULL;

	CTX.ready = false;
	CTX.init = false;

	MTY_MutexUnlock(CTX.mutex);
}

bool gfx_is_ready(void)
{
	return CTX.ready;
}

bool gfx_was_reinit(bool reset)
{
	MTY_MutexLock(CTX.mutex);

	bool reinit = CTX.reinit;

	if (reset)
		CTX.reinit = false;

	MTY_MutexUnlock(CTX.mutex);

	return reinit;
}


// EGL CTX

static void gfx_gl_ctx_destroy_members(struct gfx_gl_ctx *ctx)
{
	MTY_RendererDestroy(&ctx->renderer);

	if (ctx->display) {
		if (ctx->context) {
			eglMakeCurrent(ctx->display, 0, 0, 0);
			eglDestroyContext(ctx->display, ctx->context);
		}

		if (ctx->surface)
			eglDestroySurface(ctx->display, ctx->surface);

		eglTerminate(ctx->display);
	}

	ctx->display = 0;
	ctx->surface = 0;
	ctx->context = 0;
}

static bool gfx_gl_ctx_check(struct gfx_gl_ctx *ctx)
{
	bool r = true;

	if (!ctx->ready)
		return false;

	if (ctx->ready && ctx->init)
		goto except;

	gfx_gl_ctx_destroy_members(ctx);

	ctx->display = eglGetDisplay(0);
	eglInitialize(ctx->display, NULL, NULL);

	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLint n;
	EGLConfig config;
	eglChooseConfig(ctx->display, attribs, &config, 1, &n);

	ctx->surface = eglCreateWindowSurface(ctx->display, config, ctx->window, NULL);

	const EGLint attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	ctx->context = eglCreateContext(ctx->display, config, 0, attrib);

	eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);

	ctx->renderer = MTY_RendererCreate();

	ctx->init = true;

	except:

	if (!r)
		gfx_gl_ctx_destroy_members(ctx);

	return r;
}

struct gfx_ctx *gfx_gl_ctx_create(void *native_window, bool vsync)
{
	bool r = true;

	struct gfx_gl_ctx *ctx = &CTX;

	if (!r)
		gfx_gl_ctx_destroy((struct gfx_ctx **) &ctx);

	return (struct gfx_ctx *) ctx;
}

void gfx_gl_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) *gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	MTY_RendererDestroy(&ctx->renderer);
	gfx_gl_ctx_destroy_members(ctx);

	MTY_MutexUnlock(ctx->mutex);

	MTY_MutexDestroy(&ctx->mutex);

	memset(&CTX, 0, sizeof(struct gfx_gl_ctx));
	*gfx_ctx = NULL;
}

MTY_Device *gfx_gl_ctx_get_device(struct gfx_ctx *gfx_ctx)
{
	return NULL;
}

MTY_Context *gfx_gl_ctx_get_context(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (ctx->init && ctx->ready)
		eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);

	MTY_MutexUnlock(ctx->mutex);

	return NULL;
}

MTY_Texture *gfx_gl_ctx_get_buffer(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	return (MTY_Texture *) &ctx->fb0;
}

void gfx_gl_ctx_present(struct gfx_ctx *gfx_ctx, uint32_t interval)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (gfx_gl_ctx_check(ctx)) {
		eglSwapBuffers(ctx->display, ctx->surface);
		glFinish();
	}

	MTY_MutexUnlock(ctx->mutex);
}

void gfx_gl_ctx_draw_quad(struct gfx_ctx *gfx_ctx, const void *image, const MTY_RenderDesc *desc)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (ctx->init && ctx->ready) {
		MTY_RenderDesc mutated = *desc;
		mutated.viewWidth = ctx->width;
		mutated.viewHeight = ctx->height;

		MTY_RendererDrawQuad(ctx->renderer, MTY_GFX_GL, NULL, NULL, image, &mutated, (MTY_Texture *) &ctx->fb0);
	}

	MTY_MutexUnlock(ctx->mutex);
}

void gfx_gl_ctx_draw_ui(struct gfx_ctx *gfx_ctx, const MTY_DrawData *dd)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (ctx->init && ctx->ready)
		MTY_RendererDrawUI(ctx->renderer, MTY_GFX_GL, NULL, NULL, dd, (MTY_Texture *) &ctx->fb0);

	MTY_MutexUnlock(ctx->mutex);
}

void gfx_gl_ctx_set_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id, const void *rgba,
	uint32_t width, uint32_t height)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (ctx->init && ctx->ready)
		MTY_RendererSetUITexture(ctx->renderer, MTY_GFX_GL, NULL, NULL, id, rgba, width, height);

	MTY_MutexUnlock(ctx->mutex);
}

void *gfx_gl_ctx_get_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	void *tex = NULL;

	MTY_MutexLock(ctx->mutex);

	if (ctx->init && ctx->ready)
		tex = MTY_RendererGetUITexture(ctx->renderer, id);

	MTY_MutexUnlock(ctx->mutex);

	return tex;
}
