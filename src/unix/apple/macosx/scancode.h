// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

static MTY_Scancode keycode_to_scancode(unsigned short kc)
{
	switch (kc) {
		case kVK_ANSI_A:              return MTY_SCANCODE_A;
		case kVK_ANSI_S:              return MTY_SCANCODE_S;
		case kVK_ANSI_D:              return MTY_SCANCODE_D;
		case kVK_ANSI_F:              return MTY_SCANCODE_F;
		case kVK_ANSI_H:              return MTY_SCANCODE_H;
		case kVK_ANSI_G:              return MTY_SCANCODE_G;
		case kVK_ANSI_Z:              return MTY_SCANCODE_Z;
		case kVK_ANSI_X:              return MTY_SCANCODE_X;
		case kVK_ANSI_C:              return MTY_SCANCODE_C;
		case kVK_ANSI_V:              return MTY_SCANCODE_V;
		case kVK_ANSI_B:              return MTY_SCANCODE_B;
		case kVK_ANSI_Q:              return MTY_SCANCODE_Q;
		case kVK_ANSI_W:              return MTY_SCANCODE_W;
		case kVK_ANSI_E:              return MTY_SCANCODE_E;
		case kVK_ANSI_R:              return MTY_SCANCODE_R;
		case kVK_ANSI_Y:              return MTY_SCANCODE_Y;
		case kVK_ANSI_T:              return MTY_SCANCODE_T;
		case kVK_ANSI_1:              return MTY_SCANCODE_1;
		case kVK_ANSI_2:              return MTY_SCANCODE_2;
		case kVK_ANSI_3:              return MTY_SCANCODE_3;
		case kVK_ANSI_4:              return MTY_SCANCODE_4;
		case kVK_ANSI_6:              return MTY_SCANCODE_6;
		case kVK_ANSI_5:              return MTY_SCANCODE_6;
		case kVK_ANSI_Equal:          return MTY_SCANCODE_EQUALS;
		case kVK_ANSI_9:              return MTY_SCANCODE_9;
		case kVK_ANSI_7:              return MTY_SCANCODE_7;
		case kVK_ANSI_Minus:          return MTY_SCANCODE_MINUS;
		case kVK_ANSI_8:              return MTY_SCANCODE_8;
		case kVK_ANSI_0:              return MTY_SCANCODE_0;
		case kVK_ANSI_RightBracket:   return MTY_SCANCODE_RBRACKET;
		case kVK_ANSI_O:              return MTY_SCANCODE_O;
		case kVK_ANSI_U:              return MTY_SCANCODE_U;
		case kVK_ANSI_LeftBracket:    return MTY_SCANCODE_LBRACKET;
		case kVK_ANSI_I:              return MTY_SCANCODE_I;
		case kVK_ANSI_P:              return MTY_SCANCODE_P;
		case kVK_ANSI_L:              return MTY_SCANCODE_L;
		case kVK_ANSI_J:              return MTY_SCANCODE_J;
		case kVK_ANSI_Quote:          return MTY_SCANCODE_QUOTE;
		case kVK_ANSI_K:              return MTY_SCANCODE_K;
		case kVK_ANSI_Semicolon:      return MTY_SCANCODE_SEMICOLON;
		case kVK_ANSI_Backslash:      return MTY_SCANCODE_BACKSLASH;
		case kVK_ANSI_Comma:          return MTY_SCANCODE_COMMA;
		case kVK_ANSI_Slash:          return MTY_SCANCODE_SLASH;
		case kVK_ANSI_N:              return MTY_SCANCODE_N;
		case kVK_ANSI_M:              return MTY_SCANCODE_M;
		case kVK_ANSI_Period:         return MTY_SCANCODE_PERIOD;
		case kVK_ANSI_Grave:          return MTY_SCANCODE_GRAVE;
		case kVK_ANSI_KeypadDecimal:  return MTY_SCANCODE_NP_PERIOD;
		case kVK_ANSI_KeypadMultiply: return MTY_SCANCODE_NP_MULTIPLY;
		case kVK_ANSI_KeypadPlus:     return MTY_SCANCODE_NP_PLUS;
		case kVK_ANSI_KeypadClear:    return MTY_SCANCODE_NONE;
		case kVK_ANSI_KeypadDivide:   return MTY_SCANCODE_NP_DIVIDE;
		case kVK_ANSI_KeypadEnter:    return MTY_SCANCODE_NP_ENTER;
		case kVK_ANSI_KeypadMinus:    return MTY_SCANCODE_NP_MINUS;
		case kVK_ANSI_KeypadEquals:   return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad0:        return MTY_SCANCODE_NP_0;
		case kVK_ANSI_Keypad1:        return MTY_SCANCODE_NP_1;
		case kVK_ANSI_Keypad2:        return MTY_SCANCODE_NP_2;
		case kVK_ANSI_Keypad3:        return MTY_SCANCODE_NP_3;
		case kVK_ANSI_Keypad4:        return MTY_SCANCODE_NP_4;
		case kVK_ANSI_Keypad5:        return MTY_SCANCODE_NP_5;
		case kVK_ANSI_Keypad6:        return MTY_SCANCODE_NP_6;
		case kVK_ANSI_Keypad7:        return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad8:        return MTY_SCANCODE_NONE;
		case kVK_ANSI_Keypad9:        return MTY_SCANCODE_NONE;
		case kVK_Return:              return MTY_SCANCODE_ENTER;
		case kVK_Tab:                 return MTY_SCANCODE_TAB;
		case kVK_Space:               return MTY_SCANCODE_SPACE;
		case kVK_Delete:              return MTY_SCANCODE_BACKSPACE;
		case kVK_Escape:              return MTY_SCANCODE_ESCAPE;
		case kVK_Command:             return MTY_SCANCODE_LALT;
		case kVK_Shift:               return MTY_SCANCODE_LSHIFT;
		case kVK_CapsLock:            return MTY_SCANCODE_CAPS;
		case kVK_Option:              return MTY_SCANCODE_LWIN;
		case kVK_Control:             return MTY_SCANCODE_LCTRL;
		case kVK_RightShift:          return MTY_SCANCODE_RSHIFT;
		case kVK_RightOption:         return MTY_SCANCODE_RWIN;
		case kVK_RightControl:        return MTY_SCANCODE_RCTRL;
		case kVK_Function:            return MTY_SCANCODE_NONE;
		case kVK_F17:                 return MTY_SCANCODE_NONE;
		case kVK_VolumeUp:            return MTY_SCANCODE_NONE;
		case kVK_VolumeDown:          return MTY_SCANCODE_NONE;
		case kVK_Mute:                return MTY_SCANCODE_NONE;
		case kVK_F18:                 return MTY_SCANCODE_NONE;
		case kVK_F19:                 return MTY_SCANCODE_NONE;
		case kVK_F20:                 return MTY_SCANCODE_NONE;
		case kVK_F5:                  return MTY_SCANCODE_NONE;
		case kVK_F6:                  return MTY_SCANCODE_NONE;
		case kVK_F7:                  return MTY_SCANCODE_NONE;
		case kVK_F3:                  return MTY_SCANCODE_NONE;
		case kVK_F8:                  return MTY_SCANCODE_NONE;
		case kVK_F9:                  return MTY_SCANCODE_NONE;
		case kVK_F11:                 return MTY_SCANCODE_NONE;
		case kVK_F13:                 return MTY_SCANCODE_NONE;
		case kVK_F16:                 return MTY_SCANCODE_NONE;
		case kVK_F14:                 return MTY_SCANCODE_NONE;
		case kVK_F10:                 return MTY_SCANCODE_NONE;
		case kVK_F12:                 return MTY_SCANCODE_NONE;
		case kVK_F15:                 return MTY_SCANCODE_NONE;
		case kVK_Help:                return MTY_SCANCODE_NONE;
		case kVK_Home:                return MTY_SCANCODE_NONE;
		case kVK_PageUp:              return MTY_SCANCODE_NONE;
		case kVK_ForwardDelete:       return MTY_SCANCODE_DELETE;
		case kVK_F4:                  return MTY_SCANCODE_NONE;
		case kVK_End:                 return MTY_SCANCODE_NONE;
		case kVK_F2:                  return MTY_SCANCODE_NONE;
		case kVK_PageDown:            return MTY_SCANCODE_NONE;
		case kVK_F1:                  return MTY_SCANCODE_NONE;
		case kVK_LeftArrow:           return MTY_SCANCODE_LEFT;
		case kVK_RightArrow:          return MTY_SCANCODE_RIGHT;
		case kVK_DownArrow:           return MTY_SCANCODE_DOWN;
		case kVK_UpArrow:             return MTY_SCANCODE_UP;
		case kVK_ISO_Section:         return MTY_SCANCODE_NONE;
		case kVK_JIS_Yen:             return MTY_SCANCODE_NONE;
		case kVK_JIS_Underscore:      return MTY_SCANCODE_NONE;
		case kVK_JIS_KeypadComma:     return MTY_SCANCODE_NONE;
		case kVK_JIS_Eisu:            return MTY_SCANCODE_NONE;
		case kVK_JIS_Kana:            return MTY_SCANCODE_NONE;
		default:
			break;
	}

	return MTY_SCANCODE_NONE;
}

