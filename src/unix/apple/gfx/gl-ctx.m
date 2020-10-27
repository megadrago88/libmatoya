// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_gl_)

#include <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

struct gfx_gl_ctx {
	NSOpenGLContext *gl;
	uint32_t interval;
	uint32_t fb0;
};

struct gfx_ctx *gfx_gl_ctx_create(void *native_window, bool vsync)
{
	struct gfx_gl_ctx *ctx = MTY_Alloc(1, sizeof(struct gfx_gl_ctx));
	NSWindow *window = (__bridge NSWindow *) native_window;

	NSOpenGLPixelFormatAttribute attrs[] = {
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFATripleBuffer,
		0
	};

	NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	ctx->gl = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
	[ctx->gl setView:window.contentView];
	[ctx->gl makeCurrentContext];
	[ctx->gl update];

	return (struct gfx_ctx *) ctx;
}

void gfx_gl_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) *gfx_ctx;

	ctx->gl = nil;

	MTY_Free(ctx);
	*gfx_ctx = NULL;
}

MTY_Device *gfx_gl_ctx_get_device(struct gfx_ctx *gfx_ctx)
{
	return NULL;
}

MTY_Context *gfx_gl_ctx_get_context(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	return (__bridge MTY_Context *) ctx->gl;
}

MTY_Texture *gfx_gl_ctx_get_buffer(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	return (MTY_Texture *) &ctx->fb0;
}

bool gfx_gl_ctx_refresh(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	[ctx->gl update];

	return true;
}

void gfx_gl_ctx_present(struct gfx_ctx *gfx_ctx, uint32_t interval)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	if (interval != ctx->interval) {
		[ctx->gl setValues:(GLint *) &interval forParameter:NSOpenGLCPSwapInterval];
		ctx->interval = interval;
	}

	[ctx->gl flushBuffer];
	glFinish();
}

bool gfx_gl_ctx_set_current(struct gfx_ctx *gfx_ctx)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	[ctx->gl makeCurrentContext];

	return true;
}
