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

struct gfx;

#define GFX_PROTO(func) func
#define GFX_FP(func)    (*func)

#define GFX_DECLARE_API(api, wrap) \
	struct gfx *wrap(gfx##api##create)(MTY_Device *device); \
	void wrap(gfx##api##destroy)(struct gfx **gfx); \
	bool wrap(gfx##api##render)(struct gfx *gfx, MTY_Device *device, MTY_Context *context, \
		const void *image, const MTY_RenderDesc *desc, MTY_Texture *dest); \
	void *wrap(gfx##api##get_state)(MTY_Device *device, MTY_Context *context); \
	void wrap(gfx##api##set_state)(MTY_Device *device, MTY_Context *context, void *state); \
	void wrap(gfx##api##free_state)(void **state);

#define GFX_PROTOTYPES(api) \
	GFX_DECLARE_API(api, GFX_PROTO)

#define GFX_DECLARE_ROW(API, api) \
	[MTY_GFX_##API] = { \
		gfx##api##create, \
		gfx##api##destroy, \
		gfx##api##render, \
		gfx##api##get_state, \
		gfx##api##set_state, \
		gfx##api##free_state, \
	},
