// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_gl_)

#include "../web.h"

struct gfx_gl_ctx {
	MTY_Renderer *renderer;
	uint32_t fb0;
};

static void gfx_gl_ctx_get_size(struct gfx_gl_ctx *ctx, uint32_t *width, uint32_t *height)
{
	web_get_size(width, height);
}

struct gfx_ctx *gfx_gl_ctx_create(void *native_window, bool vsync)
{
	struct gfx_gl_ctx *ctx = MTY_Alloc(1, sizeof(struct gfx_gl_ctx));
	ctx->renderer = MTY_RendererCreate();

	return (struct gfx_ctx *) ctx;
}

void gfx_gl_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) *gfx_ctx;

	MTY_RendererDestroy(&ctx->renderer);

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
}

void gfx_gl_ctx_draw_quad(struct gfx_ctx *gfx_ctx, const void *image, const MTY_RenderDesc *desc)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_RenderDesc mutated = *desc;
	gfx_gl_ctx_get_size(ctx, &mutated.viewWidth, &mutated.viewHeight);

	MTY_RendererDrawQuad(ctx->renderer, MTY_GFX_GL, NULL, NULL, image, &mutated, (MTY_Texture *) &ctx->fb0);
}

void gfx_gl_ctx_draw_ui(struct gfx_ctx *gfx_ctx, const MTY_DrawData *dd)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_RendererDrawUI(ctx->renderer, MTY_GFX_GL, NULL, NULL, dd, (MTY_Texture *) &ctx->fb0);
}

void gfx_gl_ctx_set_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id, const void *rgba,
	uint32_t width, uint32_t height)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	MTY_RendererSetUITexture(ctx->renderer, MTY_GFX_GL, NULL, NULL, id, rgba, width, height);
}

void *gfx_gl_ctx_get_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id)
{
	struct gfx_gl_ctx *ctx = (struct gfx_gl_ctx *) gfx_ctx;

	return MTY_RendererGetUITexture(ctx->renderer, id);
}
