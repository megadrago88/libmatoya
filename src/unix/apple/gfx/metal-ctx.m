// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_metal_)

#include <Metal/Metal.h>
#include <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>

struct gfx_metal_ctx {
	NSWindow *window;
	CAMetalLayer *layer;
	CVDisplayLinkRef display_link;
	dispatch_semaphore_t semaphore;
	id<CAMetalDrawable> back_buffer;
	id<MTLCommandQueue> cq;
	MTY_Renderer *renderer;
	CGSize size;
};

static CGSize gfx_metal_ctx_get_size(struct gfx_metal_ctx *ctx)
{
	CGSize size = ctx->window.contentView.frame.size;
	float scale = ctx->window.screen.backingScaleFactor;

	size.width *= scale;
	size.height *= scale;

	return size;
}

static CVReturn gfx_metal_ctx_display_link(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow,
	const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) displayLinkContext;

	dispatch_semaphore_signal(ctx->semaphore);

	return 0;
}

struct gfx_ctx *gfx_metal_ctx_create(void *native_window, bool vsync)
{
	struct gfx_metal_ctx *ctx = MTY_Alloc(1, sizeof(struct gfx_metal_ctx));
	ctx->window = (__bridge NSWindow *) native_window;
	ctx->renderer = MTY_RendererCreate();

	bool r = true;

	ctx->layer = [CAMetalLayer layer];
	ctx->layer.device = MTLCreateSystemDefaultDevice();
	ctx->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

	if (@available(macOS 10.13, *))
		ctx->layer.displaySyncEnabled = vsync ? YES : NO;

	ctx->cq = [ctx->layer.device newCommandQueue];

	ctx->window.contentView.wantsLayer = YES;
	ctx->window.contentView.layer = ctx->layer;

	CVReturn e = CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &ctx->display_link);
	if (e != 0) {
		r = false;
		goto except;
	}

	ctx->semaphore = dispatch_semaphore_create(0);

	CVDisplayLinkSetOutputCallback(ctx->display_link, gfx_metal_ctx_display_link, ctx);
	CVDisplayLinkStart(ctx->display_link);

	except:

	if (!r)
		gfx_metal_ctx_destroy((struct gfx_ctx **) &ctx);

	return (struct gfx_ctx *) ctx;
}

void gfx_metal_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) *gfx_ctx;

	MTY_RendererDestroy(&ctx->renderer);

	ctx->layer = nil;
	ctx->cq = nil;
	ctx->semaphore = nil;
	ctx->back_buffer = nil;

	if (ctx->window) {
		ctx->window.contentView.layer = nil;
		ctx->window.contentView.wantsLayer = NO;
		ctx->window = nil;
	}

	if (ctx->display_link) {
		if (CVDisplayLinkIsRunning(ctx->display_link))
			CVDisplayLinkStop(ctx->display_link);

		CVDisplayLinkRelease(ctx->display_link);
	}

	MTY_Free(ctx);
	*gfx_ctx = NULL;
}

MTY_Device *gfx_metal_ctx_get_device(struct gfx_ctx *gfx_ctx)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	return (__bridge MTY_Device *) ctx->cq.device;
}

MTY_Context *gfx_metal_ctx_get_context(struct gfx_ctx *gfx_ctx)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	return (__bridge MTY_Context *) ctx->cq;
}

static void gfx_metal_ctx_refresh(struct gfx_metal_ctx *ctx)
{
	CGSize size = gfx_metal_ctx_get_size(ctx);

	if (size.width != ctx->size.width || size.height != ctx->size.height) {
		ctx->layer.drawableSize = size;
		ctx->size = size;
	}
}

MTY_Texture *gfx_metal_ctx_get_buffer(struct gfx_ctx *gfx_ctx)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	@autoreleasepool {
		if (!ctx->back_buffer) {
			gfx_metal_ctx_refresh(ctx);
			ctx->back_buffer = [ctx->layer nextDrawable];
		}
	}

	return (__bridge MTY_Texture *) ctx->back_buffer.texture;
}

void gfx_metal_ctx_present(struct gfx_ctx *gfx_ctx, uint32_t interval)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	if (ctx->back_buffer) {
		for (uint32_t x = 0; x < interval; x++)
			dispatch_semaphore_wait(ctx->semaphore, DISPATCH_TIME_FOREVER);

		id<MTLCommandBuffer> cb = [ctx->cq commandBuffer];
		[cb presentDrawable:ctx->back_buffer];
		[cb commit];

		ctx->back_buffer = nil;
	}
}

void gfx_metal_ctx_draw_quad(struct gfx_ctx *gfx_ctx, const void *image, const MTY_RenderDesc *desc)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	gfx_metal_ctx_get_buffer(gfx_ctx);

	if (ctx->back_buffer) {
		MTY_RenderDesc mutated = *desc;
		mutated.viewWidth = lrint(ctx->size.width);
		mutated.viewHeight = lrint(ctx->size.height);

		MTY_RendererDrawQuad(ctx->renderer, MTY_GFX_METAL, (__bridge MTY_Device *) ctx->cq.device,
			(__bridge MTY_Context *) ctx->cq, image, &mutated, (__bridge MTY_Texture *) ctx->back_buffer.texture);
	}
}

void gfx_metal_ctx_draw_ui(struct gfx_ctx *gfx_ctx, const MTY_DrawData *dd)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	gfx_metal_ctx_get_buffer(gfx_ctx);

	if (ctx->back_buffer)
		MTY_RendererDrawUI(ctx->renderer, MTY_GFX_METAL, (__bridge MTY_Device *) ctx->cq.device,
			(__bridge MTY_Context *) ctx->cq, dd, (__bridge MTY_Texture *) ctx->back_buffer.texture);
}

void gfx_metal_ctx_set_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id, const void *rgba,
	uint32_t width, uint32_t height)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	MTY_RendererSetUITexture(ctx->renderer, MTY_GFX_METAL, (__bridge MTY_Device *) ctx->cq.device,
		(__bridge MTY_Context *) ctx->cq, id, rgba, width, height);
}

void *gfx_metal_ctx_get_ui_texture(struct gfx_ctx *gfx_ctx, uint32_t id)
{
	struct gfx_metal_ctx *ctx = (struct gfx_metal_ctx *) gfx_ctx;

	return MTY_RendererGetUITexture(ctx->renderer, id);
}
