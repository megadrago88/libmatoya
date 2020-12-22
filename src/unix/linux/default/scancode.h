// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "x-dl.h"

static MTY_Scancode window_keysym_to_scancode(KeySym sym)
{
	switch (sym) {
		case XK_a:              return MTY_SCANCODE_A;
		case XK_s:              return MTY_SCANCODE_S;
		case XK_d:              return MTY_SCANCODE_D;
		case XK_f:              return MTY_SCANCODE_F;
		case XK_h:              return MTY_SCANCODE_H;
		case XK_g:              return MTY_SCANCODE_G;
		case XK_z:              return MTY_SCANCODE_Z;
		case XK_x:              return MTY_SCANCODE_X;
		case XK_c:              return MTY_SCANCODE_C;
		case XK_v:              return MTY_SCANCODE_V;
		// case kVK_ISO_Section:         return MTY_SCANCODE_INTL_BACKSLASH;
		case XK_b:              return MTY_SCANCODE_B;
		case XK_q:              return MTY_SCANCODE_Q;
		case XK_w:              return MTY_SCANCODE_W;
		case XK_e:              return MTY_SCANCODE_E;
		case XK_r:              return MTY_SCANCODE_R;
		case XK_y:              return MTY_SCANCODE_Y;
		case XK_t:              return MTY_SCANCODE_T;
		case XK_1:              return MTY_SCANCODE_1;
		case XK_2:              return MTY_SCANCODE_2;
		case XK_3:              return MTY_SCANCODE_3;
		case XK_4:              return MTY_SCANCODE_4;
		case XK_6:              return MTY_SCANCODE_6;
		case XK_5:              return MTY_SCANCODE_5;
		// case kVK_ANSI_Equal:          return MTY_SCANCODE_EQUALS;
		case XK_9:              return MTY_SCANCODE_9;
		case XK_7:              return MTY_SCANCODE_7;
		// case kVK_ANSI_Minus:          return MTY_SCANCODE_MINUS;
		case XK_8:              return MTY_SCANCODE_8;
		case XK_0:              return MTY_SCANCODE_0;
		// case kVK_ANSI_RightBracket:   return MTY_SCANCODE_RBRACKET;
		case XK_o:              return MTY_SCANCODE_O;
		case XK_u:              return MTY_SCANCODE_U;
		// case kVK_ANSI_LeftBracket:    return MTY_SCANCODE_LBRACKET;
		case XK_i:              return MTY_SCANCODE_I;
		case XK_p:              return MTY_SCANCODE_P;
		case XK_Return:         return MTY_SCANCODE_ENTER;
		case XK_l:              return MTY_SCANCODE_L;
		case XK_j:              return MTY_SCANCODE_J;
		// case kVK_ANSI_Quote:          return MTY_SCANCODE_QUOTE;
		case XK_k:              return MTY_SCANCODE_K;
		case XK_semicolon:      return MTY_SCANCODE_SEMICOLON;
		// case kVK_ANSI_Backslash:      return MTY_SCANCODE_BACKSLASH;
		// case kVK_ANSI_Comma:          return MTY_SCANCODE_COMMA;
		// case kVK_ANSI_Slash:          return MTY_SCANCODE_SLASH;
		case XK_n:              return MTY_SCANCODE_N;
		case XK_m:              return MTY_SCANCODE_M;
		// case kVK_ANSI_Period:         return MTY_SCANCODE_PERIOD;
		case XK_Tab:            return MTY_SCANCODE_TAB;
		case XK_space:          return MTY_SCANCODE_SPACE;
		// case kVK_ANSI_Grave:          return MTY_SCANCODE_GRAVE;
		case XK_BackSpace:      return MTY_SCANCODE_BACKSPACE;
		case XK_Escape:         return MTY_SCANCODE_ESCAPE;
		// case kVK_RightCommand:        return MTY_SCANCODE_RWIN;
		// case kVK_Command:             return MTY_SCANCODE_LWIN;
		case XK_Shift_L:        return MTY_SCANCODE_LSHIFT;
		// case kVK_CapsLock:            return MTY_SCANCODE_CAPS;
		case XK_Alt_L:          return MTY_SCANCODE_LALT;
		case XK_Control_L:      return MTY_SCANCODE_LCTRL;
		case XK_Shift_R:        return MTY_SCANCODE_RSHIFT;
		case XK_Alt_R:          return MTY_SCANCODE_RALT;
		case XK_Control_R:      return MTY_SCANCODE_RCTRL;
		// case kVK_F17:                 return MTY_SCANCODE_F17;
		// case kVK_ANSI_KeypadDecimal:  return MTY_SCANCODE_NP_PERIOD;
		// case kVK_ANSI_KeypadMultiply: return MTY_SCANCODE_NP_MULTIPLY;
		// case kVK_ANSI_KeypadPlus:     return MTY_SCANCODE_NP_PLUS;
		// case kVK_ANSI_KeypadClear:    return MTY_SCANCODE_NUM_LOCK;
		// case kVK_VolumeUp:            return MTY_SCANCODE_VOLUME_UP;
		// case kVK_VolumeDown:          return MTY_SCANCODE_VOLUME_DOWN;
		// case kVK_Mute:                return MTY_SCANCODE_MUTE;
		// case kVK_ANSI_KeypadDivide:   return MTY_SCANCODE_NP_DIVIDE;
		// case kVK_ANSI_KeypadEnter:    return MTY_SCANCODE_NP_ENTER;
		// case kVK_ANSI_KeypadMinus:    return MTY_SCANCODE_NP_MINUS;
		// case kVK_F18:                 return MTY_SCANCODE_F18;
		// case kVK_F19:                 return MTY_SCANCODE_F19;
		// case kVK_ANSI_KeypadEquals:   return MTY_SCANCODE_EQUALS;
		// case kVK_ANSI_Keypad0:        return MTY_SCANCODE_NP_0;
		// case kVK_ANSI_Keypad1:        return MTY_SCANCODE_NP_1;
		// case kVK_ANSI_Keypad2:        return MTY_SCANCODE_NP_2;
		// case kVK_ANSI_Keypad3:        return MTY_SCANCODE_NP_3;
		// case kVK_ANSI_Keypad4:        return MTY_SCANCODE_NP_4;
		// case kVK_ANSI_Keypad5:        return MTY_SCANCODE_NP_5;
		// case kVK_ANSI_Keypad6:        return MTY_SCANCODE_NP_6;
		// case kVK_ANSI_Keypad7:        return MTY_SCANCODE_NP_7;
		// case kVK_ANSI_Keypad8:        return MTY_SCANCODE_NP_8;
		// case kVK_ANSI_Keypad9:        return MTY_SCANCODE_NP_9;
		// case kVK_JIS_Yen:             return MTY_SCANCODE_YEN;
		// case kVK_JIS_Underscore:      return MTY_SCANCODE_RO;
		// case kVK_JIS_KeypadComma:     return MTY_SCANCODE_INTL_COMMA;
		// case kVK_F5:                  return MTY_SCANCODE_F5;
		// case kVK_F6:                  return MTY_SCANCODE_F6;
		// case kVK_F7:                  return MTY_SCANCODE_F7;
		// case kVK_F3:                  return MTY_SCANCODE_F3;
		// case kVK_F8:                  return MTY_SCANCODE_F8;
		// case kVK_F9:                  return MTY_SCANCODE_F9;
		// case kVK_JIS_Eisu:            return MTY_SCANCODE_MUHENKAN;
		// case kVK_F11:                 return MTY_SCANCODE_F11;
		// case kVK_JIS_Kana:            return MTY_SCANCODE_HENKAN;
		// case kVK_F13:                 return MTY_SCANCODE_PRINT_SCREEN;
		// case kVK_F16:                 return MTY_SCANCODE_F16;
		// case kVK_F14:                 return MTY_SCANCODE_SCROLL_LOCK;
		// case kVK_F10:                 return MTY_SCANCODE_F10;
		// case kVK_WinApp:              return MTY_SCANCODE_APP;
		// case kVK_F12:                 return MTY_SCANCODE_F12;
		// case kVK_F15:                 return MTY_SCANCODE_PAUSE;
		case XK_Insert:         return MTY_SCANCODE_INSERT;
		case XK_Home:           return MTY_SCANCODE_HOME;
		case XK_Page_Up:        return MTY_SCANCODE_PAGE_UP;
		case XK_Delete:         return MTY_SCANCODE_DELETE;
		// case kVK_F4:                  return MTY_SCANCODE_F4;
		case XK_End:            return MTY_SCANCODE_END;
		// case kVK_F2:                  return MTY_SCANCODE_F2;
		case XK_Page_Down:      return MTY_SCANCODE_PAGE_DOWN;
		// case kVK_F1:                  return MTY_SCANCODE_F1;
		case XK_Left:           return MTY_SCANCODE_LEFT;
		case XK_Right:          return MTY_SCANCODE_RIGHT;
		case XK_Down:           return MTY_SCANCODE_DOWN;
		case XK_Up:             return MTY_SCANCODE_UP;
	}

	return MTY_SCANCODE_NONE;
}
