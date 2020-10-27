// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod-ctx.h"
GFX_CTX_PROTOTYPES(_d3d9_)

#define COBJMACROS
#include <d3d9.h>

#define D3D_FATAL(e) ( \
	(e) == D3DERR_DEVICEREMOVED  || \
	(e) == D3DERR_DEVICELOST     || \
	(e) == D3DERR_DEVICEHUNG     || \
	(e) == D3DERR_DEVICENOTRESET \
)

struct gfx_d3d9_ctx {
	HWND hwnd;
	bool vsync;
	IDirect3D9Ex *factory;
	IDirect3DDevice9Ex *device;
	IDirect3DDevice9 *device_og;
	IDirect3DSwapChain9Ex *swap_chain;
	IDirect3DSurface9 *back_buffer;
};

static bool gfx_d3d9_ctx_init(struct gfx_d3d9_ctx *ctx)
{
	IDirect3DSwapChain9 *swap_chain = NULL;

	HRESULT e = Direct3DCreate9Ex(D3D_SDK_VERSION, &ctx->factory);
	if (e != S_OK)
		goto except;

	D3DPRESENT_PARAMETERS pp = {0};
	pp.BackBufferFormat = D3DFMT_UNKNOWN;
	pp.BackBufferCount = 1;
	pp.SwapEffect = D3DSWAPEFFECT_FLIPEX;
	pp.hDeviceWindow = ctx->hwnd;
	pp.Windowed = TRUE;

	DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE |
		D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_DISABLE_PSGP_THREADING;

	e = IDirect3D9Ex_CreateDeviceEx(ctx->factory, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, ctx->hwnd,
		flags, &pp, NULL, &ctx->device);
	if (e != S_OK)
		goto except;

	e = IDirect3DDevice9Ex_SetMaximumFrameLatency(ctx->device, 1);
	if (e != S_OK)
		goto except;

	e = IDirect3DDevice9Ex_QueryInterface(ctx->device, &IID_IDirect3DDevice9, (void **) &ctx->device_og);
	if (e != S_OK)
		goto except;

	e = IDirect3DDevice9Ex_GetSwapChain(ctx->device, 0, &swap_chain);
	if (e != S_OK)
		goto except;

	e = IDirect3DSwapChain9_QueryInterface(swap_chain, &IID_IDirect3DSwapChain9Ex, (void **) &ctx->swap_chain);
	if (e != S_OK)
		goto except;

	except:

	if (swap_chain)
		IDirect3DSwapChain9_Release(swap_chain);

	return e == S_OK;
}

struct gfx_ctx *gfx_d3d9_ctx_create(void *native_window, bool vsync)
{
	struct gfx_d3d9_ctx *ctx = MTY_Alloc(1, sizeof(struct gfx_d3d9_ctx));
	ctx->hwnd = (HWND) native_window;
	ctx->vsync = vsync;

	if (!gfx_d3d9_ctx_init(ctx))
		gfx_d3d9_ctx_destroy((struct gfx_ctx **) &ctx);

	return (struct gfx_ctx *) ctx;
}

static void gfx_d3d9_ctx_free(struct gfx_d3d9_ctx *ctx)
{
	if (ctx->back_buffer)
		IDirect3DSurface9_Release(ctx->back_buffer);

	if (ctx->swap_chain)
		IDirect3DSwapChain9Ex_Release(ctx->swap_chain);

	if (ctx->device_og)
		IDirect3DDevice9_Release(ctx->device_og);

	if (ctx->device)
		IDirect3DDevice9Ex_Release(ctx->device);

	if (ctx->factory)
		IDirect3D9Ex_Release(ctx->factory);

	ctx->back_buffer = NULL;
	ctx->swap_chain = NULL;
	ctx->device_og = NULL;
	ctx->device = NULL;
	ctx->factory = NULL;
}

void gfx_d3d9_ctx_destroy(struct gfx_ctx **gfx_ctx)
{
	if (!gfx_ctx || !*gfx_ctx)
		return;

	struct gfx_d3d9_ctx *ctx = (struct gfx_d3d9_ctx *) *gfx_ctx;

	gfx_d3d9_ctx_free(ctx);

	MTY_Free(ctx);
	*gfx_ctx = NULL;
}

MTY_Device *gfx_d3d9_ctx_get_device(struct gfx_ctx *gfx_ctx)
{
	struct gfx_d3d9_ctx *ctx = (struct gfx_d3d9_ctx *) gfx_ctx;

	return (MTY_Device *) ctx->device_og;
}

MTY_Context *gfx_d3d9_ctx_get_context(struct gfx_ctx *gfx_ctx)
{
	return NULL;
}

MTY_Texture *gfx_d3d9_ctx_get_buffer(struct gfx_ctx *gfx_ctx)
{
	struct gfx_d3d9_ctx *ctx = (struct gfx_d3d9_ctx *) gfx_ctx;

	if (!ctx->back_buffer) {
		HRESULT e = IDirect3DSwapChain9Ex_GetBackBuffer(ctx->swap_chain, 0,
			D3DBACKBUFFER_TYPE_MONO, &ctx->back_buffer);
		if (e != S_OK)
			goto except;

		e = IDirect3DDevice9Ex_BeginScene(ctx->device);
		if (e != S_OK)
			goto except;

		except:

		if (e != S_OK) {
			if (ctx->back_buffer) {
				IDirect3DSurface9_Release(ctx->back_buffer);
				ctx->back_buffer = NULL;
			}
		}
	}

	return (MTY_Texture *) ctx->back_buffer;
}

bool gfx_d3d9_ctx_refresh(struct gfx_ctx *gfx_ctx)
{
	struct gfx_d3d9_ctx *ctx = (struct gfx_d3d9_ctx *) gfx_ctx;

	D3DPRESENT_PARAMETERS pp = {0};
	pp.BackBufferFormat = D3DFMT_UNKNOWN;
	pp.BackBufferCount = 1;
	pp.SwapEffect = D3DSWAPEFFECT_FLIPEX;
	pp.hDeviceWindow = ctx->hwnd;
	pp.Windowed = TRUE;

	HRESULT e = IDirect3DDevice9Ex_ResetEx(ctx->device, &pp, NULL);

	if (e != S_OK)
		MTY_Log("'IDirect3DDevice9Ex_ResetEx' failed with HRESULT 0x%X", e);

	if (D3D_FATAL(e)) {
		gfx_d3d9_ctx_free(ctx);
		gfx_d3d9_ctx_init(ctx);
	}

	return e == S_OK;
}

void gfx_d3d9_ctx_present(struct gfx_ctx *gfx_ctx, uint32_t interval)
{
	struct gfx_d3d9_ctx *ctx = (struct gfx_d3d9_ctx *) gfx_ctx;

	if (ctx->back_buffer) {
		HRESULT e = IDirect3DDevice9Ex_EndScene(ctx->device);

		if (e == S_OK) {
			e = IDirect3DDevice9Ex_PresentEx(ctx->device, NULL, NULL, NULL, NULL, 0);

			if (D3D_FATAL(e)) {
				gfx_d3d9_ctx_free(ctx);
				gfx_d3d9_ctx_init(ctx);
			}
		}

		if (ctx->back_buffer) {
			IDirect3DSurface9_Release(ctx->back_buffer);
			ctx->back_buffer = NULL;
		}
	}
}
