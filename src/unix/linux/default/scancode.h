// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "x-dl.h"

static MTY_Scancode window_x_to_mty(KeySym sym)
{
	switch (sym) {
		case XK_Escape: return MTY_SCANCODE_ESCAPE;
		case XK_Tab: return MTY_SCANCODE_TAB;
		case XK_w: return MTY_SCANCODE_W;
		case XK_c: return MTY_SCANCODE_C;
		case XK_x: return MTY_SCANCODE_X;
		case XK_y: return MTY_SCANCODE_Y;
		case XK_z: return MTY_SCANCODE_Z;
		case XK_v: return MTY_SCANCODE_V;
		case XK_r: return MTY_SCANCODE_R;
		case XK_a: return MTY_SCANCODE_A;
		case XK_s: return MTY_SCANCODE_S;
		case XK_d: return MTY_SCANCODE_D;
		case XK_l: return MTY_SCANCODE_L;
		case XK_o: return MTY_SCANCODE_O;
		case XK_p: return MTY_SCANCODE_P;
		case XK_m: return MTY_SCANCODE_M;
		case XK_t: return MTY_SCANCODE_T;
		case XK_1: return MTY_SCANCODE_1;
		case XK_semicolon: return MTY_SCANCODE_SEMICOLON;
		case XK_Shift_L: return MTY_SCANCODE_LSHIFT;
		case XK_space: return MTY_SCANCODE_SPACE;
		case XK_Left: return MTY_SCANCODE_LEFT;
		case XK_Right: return MTY_SCANCODE_RIGHT;
		case XK_Up: return MTY_SCANCODE_UP;
		case XK_Down: return MTY_SCANCODE_DOWN;
		case XK_Page_Up: return MTY_SCANCODE_PAGE_UP;
		case XK_Page_Down: return MTY_SCANCODE_PAGE_DOWN;
		case XK_Return: return MTY_SCANCODE_ENTER;
		case XK_Home: return MTY_SCANCODE_HOME;
		case XK_End: return MTY_SCANCODE_END;
		case XK_Insert: return MTY_SCANCODE_INSERT;
		case XK_Delete: return MTY_SCANCODE_DELETE;
		case XK_BackSpace: return MTY_SCANCODE_BACKSPACE;
		case XK_Shift_R: return MTY_SCANCODE_RSHIFT;
		case XK_Control_L: return MTY_SCANCODE_LCTRL;
		case XK_Control_R: return MTY_SCANCODE_RCTRL;
		case XK_Alt_L: return MTY_SCANCODE_LALT;
		case XK_Alt_R: return MTY_SCANCODE_RALT;
		// case XK_: return MTY_SCANCODE_LGUI;
		// case XK_: return MTY_SCANCODE_RGUI;
	}

	return MTY_SCANCODE_NONE;
}
