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

struct gfx_ui;

#define GFX_UI_PROTO(func) func
#define GFX_UI_FP(func)    (*func)

#define GFX_UI_DECLARE_API(api, wrap)                                              \
	struct gfx_ui *wrap(gfx##api##ui_create)(MTY_Device *device);                  \
	void wrap(gfx##api##ui_destroy)(struct gfx_ui **gfx_ui);                       \
	bool wrap(gfx##api##ui_render)(struct gfx_ui *gfx_ui, MTY_Device *device,      \
		MTY_Context *context, const MTY_DrawData *dd, MTY_Hash *cache,             \
		MTY_Texture *dest);                                                        \
	void *wrap(gfx##api##ui_create_texture)(MTY_Device *device, const void *rgba,  \
		uint32_t width, uint32_t height);                                          \
	void wrap(gfx##api##ui_destroy_texture)(void **texture);

#define GFX_UI_PROTOTYPES(api) \
	GFX_UI_DECLARE_API(api, GFX_UI_PROTO)

#define GFX_UI_DECLARE_ROW(API, api)   \
	[MTY_GFX_##API] = {                \
		gfx##api##ui_create,           \
		gfx##api##ui_destroy,          \
		gfx##api##ui_render,           \
		gfx##api##ui_create_texture,   \
		gfx##api##ui_destroy_texture,  \
	},
