// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "matoya.h"
#include "gfx/mod-support.h"

struct gfx_ctx;

#define GFX_CTX_PROTO(func) func
#define GFX_CTX_FP(func)    (*func)

#define GFX_CTX_DECLARE_API(api, wrap) \
	struct gfx_ctx *wrap(gfx##api##ctx_create)(void *native_window, bool vsync); \
	void wrap(gfx##api##ctx_destroy)(struct gfx_ctx **gfx_ctx); \
	bool wrap(gfx##api##ctx_refresh)(struct gfx_ctx *gfx_ctx); \
	void wrap(gfx##api##ctx_present)(struct gfx_ctx *gfx_ctx, uint32_t num_frames); \
	MTY_Device *wrap(gfx##api##ctx_get_device)(struct gfx_ctx *gfx_ctx); \
	MTY_Context *wrap(gfx##api##ctx_get_context)(struct gfx_ctx *gfx_ctx); \
	MTY_Texture *wrap(gfx##api##ctx_get_buffer)(struct gfx_ctx *gfx_ctx);

#define GFX_CTX_PROTOTYPES(api) \
	GFX_CTX_DECLARE_API(api, GFX_CTX_PROTO)

#define GFX_CTX_DECLARE_ROW(API, api) \
	[MTY_GFX_##API] = { \
		gfx##api##ctx_create, \
		gfx##api##ctx_destroy, \
		gfx##api##ctx_refresh, \
		gfx##api##ctx_present, \
		gfx##api##ctx_get_device, \
		gfx##api##ctx_get_context, \
		gfx##api##ctx_get_buffer, \
	},
