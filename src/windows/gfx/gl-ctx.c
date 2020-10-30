// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_gl_)

#include <windows.h>
#include <gl/gl.h>

typedef BOOL (*PFNWGLSWAPINTERVALEXTPROC)(int interval);

struct gfx_gl_ctx {
	HWND hwnd;
	HGLRC hgl;
	HDC hdc;
	MTY_Renderer *renderer;
	uint32_t interval;
	uint32_t width;
	uint32_t height;
	uint32_t fb0;
};

static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static void gfx_gl_ctx_get_size(struct gfx_gl_ctx *ctx, uint32_t *width, uint32_t *height)
{
	RECT rect = {0};
	GetClientRect(ctx->hwnd, &rect);

	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
}

struct gfx_ctx *gfx_gl_ctx_create(void *native_window, bool vsync)
{
	struct gfx_gl_ctx *ctx = MTY_Alloc(1, sizeof(struct gfx_gl_ctx));
	ctx->hwnd = (HWND) native_window;

	bool r = true;

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DEPTH_DONTCARE |
		PFD_DOUBLEBUFFER | PFD_SWAP_LAYER_BUFFERS | PFD_SWAP_EXCHANGE;
	pfd.iPixelType = PFD_TYPE_RGBA;

	ctx->hdc = GetDC(ctx->hwnd);
	if (!ctx->hdc) {
		r = false;
		MTY_Log("'GetDC' failed");
		goto except;
	}

	int32_t pf = ChoosePixelFormat(ctx->hdc, &pfd);
	if (pf == 0) {
		r = false;
		MTY_Log("'ChoosePixelFormat' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!SetPixelFormat(ctx->hdc, pf, &pfd)) {
		r = false;
		MTY_Log("'SetPixelFormat' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->hgl = wglCreateContext(ctx->hdc);
	if (!ctx->hgl) {
		r = false;
		MTY_Log("'wglCreateContext' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!wglMakeCurrent(ctx->hdc, ctx->hgl)) {
		r = false;
		MTY_Log("'wglMakeCurrent' failed with error 0x%X", GetLastError());
		goto except;
	}

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	if (!wglSwapIntervalEXT) {
		r = false;
		MTY_Log("'wglGetProcAddress' failed to find 'wglSwapIntervalEXT'");
		goto except;
	}

	gfx_gl_ctx_get_size(ctx, &ctx->width, &ctx->height);

	except:

	if (!r)
		gfx_gl_ctx_destroy((struct gfx_ctx **) &ctx);

	return (struct gfx_ctx *) ctx;
}

void gfx_gl_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) *gfx_ctx;

	wglMakeCurrent(NULL, NULL);

	if (ctx->hgl)
		wglDeleteContext(ctx->hgl);

	if (ctx->hwnd && ctx->hdc)
		ReleaseDC(ctx->hwnd, ctx->hdc);

	MTY_Free(ctx);
	*gfx_ctx = NULL;
}

MTY_Device *gfx_gl_ctx_get_device(struct gfx_ctx *gfx_ctx)
{
	return NULL;
}

MTY_Context *gfx_gl_ctx_get_context(struct gfx_ctx *gfx_ctx)
{
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

	if (interval != ctx->interval) {
		if (!wglSwapIntervalEXT(interval))
			MTY_Log("'wglSwapIntervalEXT' failed with error 0x%X", GetLastError());

		ctx->interval = interval;
	}

	if (!wglSwapLayerBuffers(ctx->hdc, WGL_SWAP_MAIN_PLANE))
		MTY_Log("'wglSwapLayerBuffers' failed with error 0x%X", GetLastError());

	glFinish();
}
