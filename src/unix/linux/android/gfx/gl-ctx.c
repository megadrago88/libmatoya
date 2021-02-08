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
	MTY_Atomic32 state_ctr;
} CTX;


// JNI

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_gfx_1dims(JNIEnv *env, jobject obj,
	jint w, jint h)
{
	CTX.width = w;
	CTX.height = h;

	MTY_Atomic32Set(&CTX.state_ctr, 2);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_gfx_1set_1surface(JNIEnv *env, jobject obj,
	jobject surface)
{
	MTY_MutexLock(CTX.mutex);

	CTX.window = ANativeWindow_fromSurface(env, surface);

	if (CTX.window) {
		CTX.ready = true;
		CTX.reinit = true;
	}

	MTY_MutexUnlock(CTX.mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_gfx_1unset_1surface(JNIEnv *env, jobject obj)
{
	MTY_MutexLock(CTX.mutex);

	if (CTX.surface)
		eglDestroySurface(CTX.display, CTX.surface);

	if (CTX.window)
		ANativeWindow_release(CTX.window);

	CTX.surface = EGL_NO_SURFACE;
	CTX.window = NULL;

	CTX.ready = false;
	CTX.init = false;

	MTY_MutexUnlock(CTX.mutex);
}

void gfx_global_init(void)
{
	CTX.mutex = MTY_MutexCreate();
}

void gfx_global_destroy(void)
{
	MTY_MutexDestroy(&CTX.mutex);

	memset(&CTX, 0, sizeof(struct gfx_gl_ctx));
}

bool gfx_is_ready(void)
{
	return CTX.ready;
}

uint32_t gfx_width(void)
{
	return CTX.width;
}

uint32_t gfx_height(void)
{
	return CTX.height;
}

MTY_GFXState gfx_state(void)
{
	MTY_GFXState state = MTY_GFX_STATE_NORMAL;

	MTY_MutexLock(CTX.mutex);

	if (CTX.reinit) {
		state = MTY_GFX_STATE_NEW_CONTEXT;
		CTX.reinit = false;

	} else if (MTY_Atomic32Get(&CTX.state_ctr) > 0) {
		MTY_Atomic32Add(&CTX.state_ctr, -1);
		state = MTY_GFX_STATE_REFRESH;
	}

	MTY_MutexUnlock(CTX.mutex);

	return state;
}


// EGL CTX

static void gfx_gl_ctx_create_context(struct gfx_gl_ctx *ctx)
{
	ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx->display, NULL, NULL);

	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLint n = 0;
	EGLConfig config = NULL;
	eglChooseConfig(ctx->display, attribs, &config, 1, &n);

	ctx->surface = eglCreateWindowSurface(ctx->display, config, ctx->window, NULL);

	const EGLint attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	ctx->context = eglCreateContext(ctx->display, config, EGL_NO_CONTEXT, attrib);

	eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);
}

static void gfx_gl_ctx_destroy_context(struct gfx_gl_ctx *ctx)
{
	if (ctx->display) {
		if (ctx->context) {
			eglMakeCurrent(ctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(ctx->display, ctx->context);
		}

		if (ctx->surface)
			eglDestroySurface(ctx->display, ctx->surface);

		eglTerminate(ctx->display);
	}

	ctx->display = EGL_NO_DISPLAY;
	ctx->surface = EGL_NO_SURFACE;
	ctx->context = EGL_NO_CONTEXT;
}


// GFX

static bool gfx_gl_ctx_check(struct gfx_gl_ctx *ctx)
{
	if (!ctx->ready)
		return false;

	if (ctx->init)
		return true;

	MTY_RendererDestroy(&ctx->renderer);
	gfx_gl_ctx_destroy_context(ctx);

	gfx_gl_ctx_create_context(ctx);
	ctx->renderer = MTY_RendererCreate();

	ctx->init = true;

	return true;
}

struct gfx_ctx *gfx_gl_ctx_create(void *native_window, bool vsync)
{
	return (struct gfx_ctx *) &CTX;
}

void gfx_gl_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) *gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	MTY_RendererDestroy(&ctx->renderer);
	gfx_gl_ctx_destroy_context(ctx);

	MTY_MutexUnlock(ctx->mutex);

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

	if (gfx_gl_ctx_check(ctx))
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

	if (gfx_gl_ctx_check(ctx)) {
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

	if (gfx_gl_ctx_check(ctx))
		MTY_RendererDrawUI(ctx->renderer, MTY_GFX_GL, NULL, NULL, dd, (MTY_Texture *) &ctx->fb0);

	MTY_MutexUnlock(ctx->mutex);
}

void gfx_gl_ctx_set_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id, const void *rgba,
	uint32_t width, uint32_t height)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_MutexLock(ctx->mutex);

	if (gfx_gl_ctx_check(ctx))
		MTY_RendererSetUITexture(ctx->renderer, MTY_GFX_GL, NULL, NULL, id, rgba, width, height);

	MTY_MutexUnlock(ctx->mutex);
}

void *gfx_gl_ctx_get_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	void *tex = NULL;

	MTY_MutexLock(ctx->mutex);

	if (gfx_gl_ctx_check(ctx))
		tex = MTY_RendererGetUITexture(ctx->renderer, id);

	MTY_MutexUnlock(ctx->mutex);

	return tex;
}
