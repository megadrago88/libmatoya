// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include "gfx/mod.h"
#include "gfx/mod-ui.h"

GFX_PROTOTYPES(_gl_)
GFX_PROTOTYPES(_d3d9_)
GFX_PROTOTYPES(_d3d11_)
GFX_PROTOTYPES(_metal_)
GFX_DECLARE_TABLE()

GFX_UI_PROTOTYPES(_gl_)
GFX_UI_PROTOTYPES(_d3d9_)
GFX_UI_PROTOTYPES(_d3d11_)
GFX_UI_PROTOTYPES(_metal_)
GFX_UI_DECLARE_TABLE()

struct MTY_Renderer {
	MTY_GFX api;
	MTY_Device *device;
	MTY_Hash *textures;

	struct gfx *gfx;
	struct gfx_ui *gfx_ui;
};

MTY_Renderer *MTY_RendererCreate(void)
{
	MTY_Renderer *ctx = MTY_Alloc(1, sizeof(MTY_Renderer));
	ctx->textures = MTY_HashCreate(100);

	return ctx;
}

static void render_destroy_api(MTY_Renderer *ctx, MTY_GFX api)
{
	GFX_API[api].gfx_destroy(&ctx->gfx);
	GFX_UI_API[api].gfx_ui_destroy(&ctx->gfx_ui);

	uint64_t i = 0;
	int64_t id = 0;

	while (MTY_HashNextKeyInt(ctx->textures, &i, &id)) {
		void *texture = MTY_HashPopInt(ctx->textures, id);
		GFX_UI_API[api].gfx_ui_destroy_texture(&texture);
	}

	ctx->api = MTY_GFX_NONE;
	ctx->device = NULL;
}

static bool render_create_api(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device)
{
	ctx->gfx = GFX_API[api].gfx_create(device);
	ctx->gfx_ui = GFX_UI_API[api].gfx_ui_create(device);

	if (!ctx->gfx || !ctx->gfx_ui) {
		render_destroy_api(ctx, api);
		return false;
	}

	ctx->api = api;
	ctx->device = device;

	return true;
}

static bool renderer_begin(MTY_Renderer *ctx, MTY_GFX api, MTY_Context *context, MTY_Device *device)
{
	if (!GFX_API_SUPPORTED(api)) {
		MTY_Log("MTY_GFX %d is unsupported", api);
		return false;
	}

	// Any change in API or device means we need to rebuild the state
	if (ctx->api != api || ctx->device != device)
		render_destroy_api(ctx, api);

	// Compile shaders, create buffers and staging areas
	return !ctx->gfx || !ctx->gfx_ui ? render_create_api(ctx, api, device) : true;
}

bool MTY_RendererDrawQuad(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device, MTY_Context *context,
	const void *image, const MTY_RenderDesc *desc, MTY_Texture *dest)
{
	if (!renderer_begin(ctx, api, context, device))
		return false;

	return GFX_API[api].gfx_render(ctx->gfx, device, context, image, desc, dest);
}

bool MTY_RendererDrawUI(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device,
	MTY_Context *context, const MTY_DrawData *dd, MTY_Texture *dest)
{
	if (!renderer_begin(ctx, api, context, device))
		return false;

	return GFX_UI_API[api].gfx_ui_render(ctx->gfx_ui, device, context, dd, ctx->textures, dest);
}

void *MTY_RendererSetUITexture(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device,
	MTY_Context *context, uint32_t id, const void *rgba, uint32_t width, uint32_t height)
{
	if (!renderer_begin(ctx, api, context, device))
		return false;

	void *texture = MTY_HashPopInt(ctx->textures, id);
	if (texture)
		GFX_UI_API[api].gfx_ui_destroy_texture(&texture);

	if (rgba) {
		texture = GFX_UI_API[api].gfx_ui_create_texture(device, rgba, width, height);
		MTY_HashSetInt(ctx->textures, id, texture);
	}

	return texture;
}

void *MTY_RendererGetUITexture(MTY_Renderer *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->textures, id);
}

uint32_t MTY_GetAvailableGFX(MTY_GFX *apis)
{
	uint32_t r = 0;

	for (uint32_t x = 0; x < MTY_GFX_MAX; x++)
		if (GFX_API_SUPPORTED(x))
			apis[r++] = x;

	return r;
}

void MTY_RendererDestroy(MTY_Renderer **renderer)
{
	if (!renderer || !*renderer)
		return;

	MTY_Renderer *ctx = *renderer;

	if (ctx->api != MTY_GFX_NONE)
		render_destroy_api(ctx, ctx->api);

	MTY_HashDestroy(&ctx->textures, NULL);

	MTY_Free(ctx);
	*renderer = NULL;
}
