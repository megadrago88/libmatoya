// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "x-dl.h"

static MTY_Scancode window_keysym_to_scancode(KeySym sym)
{
	switch (sym) {
		case XK_a:             return MTY_SCANCODE_A;
		case XK_s:             return MTY_SCANCODE_S;
		case XK_d:             return MTY_SCANCODE_D;
		case XK_f:             return MTY_SCANCODE_F;
		case XK_h:             return MTY_SCANCODE_H;
		case XK_g:             return MTY_SCANCODE_G;
		case XK_z:             return MTY_SCANCODE_Z;
		case XK_x:             return MTY_SCANCODE_X;
		case XK_c:             return MTY_SCANCODE_C;
		case XK_v:             return MTY_SCANCODE_V;
		// case XK_ISO_Section:  return MTY_SCANCODE_INTL_BACKSLASH;
		case XK_b:             return MTY_SCANCODE_B;
		case XK_q:             return MTY_SCANCODE_Q;
		case XK_w:             return MTY_SCANCODE_W;
		case XK_e:             return MTY_SCANCODE_E;
		case XK_r:             return MTY_SCANCODE_R;
		case XK_y:             return MTY_SCANCODE_Y;
		case XK_t:             return MTY_SCANCODE_T;
		case XK_1:             return MTY_SCANCODE_1;
		case XK_2:             return MTY_SCANCODE_2;
		case XK_3:             return MTY_SCANCODE_3;
		case XK_4:             return MTY_SCANCODE_4;
		case XK_6:             return MTY_SCANCODE_6;
		case XK_5:             return MTY_SCANCODE_5;
		case XK_equal:         return MTY_SCANCODE_EQUALS;
		case XK_9:             return MTY_SCANCODE_9;
		case XK_7:             return MTY_SCANCODE_7;
		case XK_minus:         return MTY_SCANCODE_MINUS;
		case XK_8:             return MTY_SCANCODE_8;
		case XK_0:             return MTY_SCANCODE_0;
		case XK_bracketright:  return MTY_SCANCODE_RBRACKET;
		case XK_o:             return MTY_SCANCODE_O;
		case XK_u:             return MTY_SCANCODE_U;
		case XK_bracketleft:   return MTY_SCANCODE_LBRACKET;
		case XK_i:             return MTY_SCANCODE_I;
		case XK_p:             return MTY_SCANCODE_P;
		case XK_Return:        return MTY_SCANCODE_ENTER;
		case XK_l:             return MTY_SCANCODE_L;
		case XK_j:             return MTY_SCANCODE_J;
		case XK_apostrophe:    return MTY_SCANCODE_QUOTE;
		case XK_k:             return MTY_SCANCODE_K;
		case XK_semicolon:     return MTY_SCANCODE_SEMICOLON;
		case XK_backslash:     return MTY_SCANCODE_BACKSLASH;
		case XK_comma:         return MTY_SCANCODE_COMMA;
		case XK_slash:         return MTY_SCANCODE_SLASH;
		case XK_n:             return MTY_SCANCODE_N;
		case XK_m:             return MTY_SCANCODE_M;
		case XK_period:        return MTY_SCANCODE_PERIOD;
		case XK_Tab:           return MTY_SCANCODE_TAB;
		case XK_space:         return MTY_SCANCODE_SPACE;
		case XK_grave:         return MTY_SCANCODE_GRAVE;
		case XK_BackSpace:     return MTY_SCANCODE_BACKSPACE;
		case XK_Escape:        return MTY_SCANCODE_ESCAPE;
		case XK_Super_R:       return MTY_SCANCODE_RWIN;
		case XK_Super_L:       return MTY_SCANCODE_LWIN;
		case XK_Shift_L:       return MTY_SCANCODE_LSHIFT;
		case XK_Caps_Lock:     return MTY_SCANCODE_CAPS;
		case XK_Alt_L:         return MTY_SCANCODE_LALT;
		case XK_Control_L:     return MTY_SCANCODE_LCTRL;
		case XK_Shift_R:       return MTY_SCANCODE_RSHIFT;
		case XK_Alt_R:         return MTY_SCANCODE_RALT;
		case XK_Control_R:     return MTY_SCANCODE_RCTRL;
		case XK_F17:           return MTY_SCANCODE_F17;
		case XK_KP_Delete:     return MTY_SCANCODE_NP_PERIOD;
		case XK_KP_Multiply:   return MTY_SCANCODE_NP_MULTIPLY;
		case XK_KP_Add:        return MTY_SCANCODE_NP_PLUS;
		case XK_Num_Lock:      return MTY_SCANCODE_NUM_LOCK;
		// case XK_VolumeUp:     return MTY_SCANCODE_VOLUME_UP;
		// case XK_VolumeDown:   return MTY_SCANCODE_VOLUME_DOWN;
		// case XK_Mute:         return MTY_SCANCODE_MUTE;
		case XK_KP_Divide:     return MTY_SCANCODE_NP_DIVIDE;
		case XK_KP_Enter:      return MTY_SCANCODE_NP_ENTER;
		case XK_KP_Subtract:   return MTY_SCANCODE_NP_MINUS;
		case XK_F18:           return MTY_SCANCODE_F18;
		case XK_F19:           return MTY_SCANCODE_F19;
		case XK_KP_Insert:     return MTY_SCANCODE_NP_0;
		case XK_KP_End:        return MTY_SCANCODE_NP_1;
		case XK_KP_Down:       return MTY_SCANCODE_NP_2;
		case XK_KP_Page_Down:  return MTY_SCANCODE_NP_3;
		case XK_KP_Left:       return MTY_SCANCODE_NP_4;
		case XK_KP_Begin:      return MTY_SCANCODE_NP_5;
		case XK_KP_Right:      return MTY_SCANCODE_NP_6;
		case XK_KP_Home:       return MTY_SCANCODE_NP_7;
		case XK_KP_Up:         return MTY_SCANCODE_NP_8;
		case XK_KP_Page_Up:    return MTY_SCANCODE_NP_9;
		case XK_yen:           return MTY_SCANCODE_YEN;
		case XK_Romaji:        return MTY_SCANCODE_RO;
		// case XK_KeypadComma:  return MTY_SCANCODE_INTL_COMMA;
		case XK_F5:            return MTY_SCANCODE_F5;
		case XK_F6:            return MTY_SCANCODE_F6;
		case XK_F7:            return MTY_SCANCODE_F7;
		case XK_F3:            return MTY_SCANCODE_F3;
		case XK_F8:            return MTY_SCANCODE_F8;
		case XK_F9:            return MTY_SCANCODE_F9;
		case XK_Muhenkan:      return MTY_SCANCODE_MUHENKAN;
		case XK_F11:           return MTY_SCANCODE_F11;
		case XK_Henkan:        return MTY_SCANCODE_HENKAN;
		case XK_F13:           return MTY_SCANCODE_F13;
		case XK_F16:           return MTY_SCANCODE_F16;
		case XK_F14:           return MTY_SCANCODE_F14;
		case XK_F10:           return MTY_SCANCODE_F10;
		case XK_Menu:          return MTY_SCANCODE_APP;
		case XK_F12:           return MTY_SCANCODE_F12;
		case XK_F15:           return MTY_SCANCODE_F15;
		case XK_Insert:        return MTY_SCANCODE_INSERT;
		case XK_Home:          return MTY_SCANCODE_HOME;
		case XK_Page_Up:       return MTY_SCANCODE_PAGE_UP;
		case XK_Delete:        return MTY_SCANCODE_DELETE;
		case XK_F4:            return MTY_SCANCODE_F4;
		case XK_End:           return MTY_SCANCODE_END;
		case XK_F2:            return MTY_SCANCODE_F2;
		case XK_Page_Down:     return MTY_SCANCODE_PAGE_DOWN;
		case XK_F1:            return MTY_SCANCODE_F1;
		case XK_Left:          return MTY_SCANCODE_LEFT;
		case XK_Right:         return MTY_SCANCODE_RIGHT;
		case XK_Down:          return MTY_SCANCODE_DOWN;
		case XK_Up:            return MTY_SCANCODE_UP;
		case XK_Print:         return MTY_SCANCODE_PRINT_SCREEN;
		case XK_Scroll_Lock:   return MTY_SCANCODE_SCROLL_LOCK;
		case XK_Pause:         return MTY_SCANCODE_PAUSE;
	}

	return MTY_SCANCODE_NONE;
}
