// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#define kVK_WinApp 0x6E

enum {
	NS_MOD_LCTRL  = 0x000001,
	NS_MOD_LSHIFT = 0x000002,
	NS_MOD_RSHIFT = 0x000004,
	NS_MOD_LCMD   = 0x000008,
	NS_MOD_RCMD   = 0x000010,
	NS_MOD_LALT   = 0x000020,
	NS_MOD_RALT   = 0x000040,
	NS_MOD_RCTRL  = 0x002000,
};

static MTY_Keymod window_modifier_flags_to_keymod(NSEventModifierFlags flags)
{
	MTY_Keymod mod = 0;

	mod |= (flags & NS_MOD_LCTRL) ? MTY_KEYMOD_LCTRL : 0;
	mod |= (flags & NS_MOD_RCTRL) ? MTY_KEYMOD_RCTRL : 0;
	mod |= (flags & NS_MOD_LSHIFT) ? MTY_KEYMOD_LSHIFT : 0;
	mod |= (flags & NS_MOD_RSHIFT) ? MTY_KEYMOD_RSHIFT : 0;
	mod |= (flags & NS_MOD_LCMD) ? MTY_KEYMOD_LWIN : 0;
	mod |= (flags & NS_MOD_RCMD) ? MTY_KEYMOD_RWIN : 0;
	mod |= (flags & NS_MOD_LALT) ? MTY_KEYMOD_LALT : 0;
	mod |= (flags & NS_MOD_RALT) ? MTY_KEYMOD_RALT : 0;
	mod |= (flags & NSEventModifierFlagCapsLock) ? MTY_KEYMOD_CAPS : 0;

	return mod;
}

static MTY_Scancode window_keycode_to_scancode(uint16_t kc)
{
	switch (kc) {
		case kVK_ANSI_A:              return MTY_SCANCODE_A; // 0x00
		case kVK_ANSI_S:              return MTY_SCANCODE_S; // 0x01
		case kVK_ANSI_D:              return MTY_SCANCODE_D; // 0x02
		case kVK_ANSI_F:              return MTY_SCANCODE_F; // 0x03
		case kVK_ANSI_H:              return MTY_SCANCODE_H; // 0x04
		case kVK_ANSI_G:              return MTY_SCANCODE_G; // 0x05
		case kVK_ANSI_Z:              return MTY_SCANCODE_Z; // 0x06
		case kVK_ANSI_X:              return MTY_SCANCODE_X; // 0x07
		case kVK_ANSI_C:              return MTY_SCANCODE_C; // 0x08
		case kVK_ANSI_V:              return MTY_SCANCODE_V; // 0x09
		case kVK_ISO_Section:         return MTY_SCANCODE_INTL_BACKSLASH; // 0x0A
		case kVK_ANSI_B:              return MTY_SCANCODE_B; // 0x0B
		case kVK_ANSI_Q:              return MTY_SCANCODE_Q; // 0x0C
		case kVK_ANSI_W:              return MTY_SCANCODE_W; // 0x0D
		case kVK_ANSI_E:              return MTY_SCANCODE_E; // 0x0E
		case kVK_ANSI_R:              return MTY_SCANCODE_R; // 0x0F
		case kVK_ANSI_Y:              return MTY_SCANCODE_Y; // 0x10
		case kVK_ANSI_T:              return MTY_SCANCODE_T; // 0x11
		case kVK_ANSI_1:              return MTY_SCANCODE_1; // 0x12
		case kVK_ANSI_2:              return MTY_SCANCODE_2; // 0x13
		case kVK_ANSI_3:              return MTY_SCANCODE_3; // 0x14
		case kVK_ANSI_4:              return MTY_SCANCODE_4; // 0x15
		case kVK_ANSI_6:              return MTY_SCANCODE_6; // 0x16
		case kVK_ANSI_5:              return MTY_SCANCODE_5; // 0x17
		case kVK_ANSI_Equal:          return MTY_SCANCODE_EQUALS; // 0x18
		case kVK_ANSI_9:              return MTY_SCANCODE_9; // 0x19
		case kVK_ANSI_7:              return MTY_SCANCODE_7; // 0x1A
		case kVK_ANSI_Minus:          return MTY_SCANCODE_MINUS; // 0x1B
		case kVK_ANSI_8:              return MTY_SCANCODE_8; // 0x1C
		case kVK_ANSI_0:              return MTY_SCANCODE_0; // 0x1D
		case kVK_ANSI_RightBracket:   return MTY_SCANCODE_RBRACKET; // 0x1E
		case kVK_ANSI_O:              return MTY_SCANCODE_O; // 0x1F
		case kVK_ANSI_U:              return MTY_SCANCODE_U; // 0x20
		case kVK_ANSI_LeftBracket:    return MTY_SCANCODE_LBRACKET; // 0x21
		case kVK_ANSI_I:              return MTY_SCANCODE_I; // 0x22
		case kVK_ANSI_P:              return MTY_SCANCODE_P; // 0x23
		case kVK_Return:              return MTY_SCANCODE_ENTER; // 0x24
		case kVK_ANSI_L:              return MTY_SCANCODE_L; // 0x25
		case kVK_ANSI_J:              return MTY_SCANCODE_J; // 0x26
		case kVK_ANSI_Quote:          return MTY_SCANCODE_QUOTE; // 0x27
		case kVK_ANSI_K:              return MTY_SCANCODE_K; // 0x28
		case kVK_ANSI_Semicolon:      return MTY_SCANCODE_SEMICOLON; // 0x29
		case kVK_ANSI_Backslash:      return MTY_SCANCODE_BACKSLASH; // 0x2A
		case kVK_ANSI_Comma:          return MTY_SCANCODE_COMMA; // 0x2B
		case kVK_ANSI_Slash:          return MTY_SCANCODE_SLASH; // 0x2C
		case kVK_ANSI_N:              return MTY_SCANCODE_N; // 0x2D
		case kVK_ANSI_M:              return MTY_SCANCODE_M; // 0x2E
		case kVK_ANSI_Period:         return MTY_SCANCODE_PERIOD; // 0x2F
		case kVK_Tab:                 return MTY_SCANCODE_TAB; // 0x30
		case kVK_Space:               return MTY_SCANCODE_SPACE; // 0x31
		case kVK_ANSI_Grave:          return MTY_SCANCODE_GRAVE; // 0x32
		case kVK_Delete:              return MTY_SCANCODE_BACKSPACE; // 0x33
		case 0x34:                    return MTY_SCANCODE_NONE; // 0x34
		case kVK_Escape:              return MTY_SCANCODE_ESCAPE; // 0x35
		case kVK_RightCommand:        return MTY_SCANCODE_RWIN; // 0x36
		case kVK_Command:             return MTY_SCANCODE_LWIN; // 0x37
		case kVK_Shift:               return MTY_SCANCODE_LSHIFT; // 0x38
		case kVK_CapsLock:            return MTY_SCANCODE_CAPS; // 0x39
		case kVK_Option:              return MTY_SCANCODE_LALT; // 0x3A
		case kVK_Control:             return MTY_SCANCODE_LCTRL; // 0x3B
		case kVK_RightShift:          return MTY_SCANCODE_RSHIFT; // 0x3C
		case kVK_RightOption:         return MTY_SCANCODE_RALT; // 0x3D
		case kVK_RightControl:        return MTY_SCANCODE_RCTRL; // 0x3E
		case kVK_Function:            return MTY_SCANCODE_NONE; // 0x3F
		case kVK_F17:                 return MTY_SCANCODE_F17; // 0x40
		case kVK_ANSI_KeypadDecimal:  return MTY_SCANCODE_NP_PERIOD; // 0x41
		case 0x42:                    return MTY_SCANCODE_NONE; // 0x42
		case kVK_ANSI_KeypadMultiply: return MTY_SCANCODE_NP_MULTIPLY; // 0x43
		case 0x44:                    return MTY_SCANCODE_NONE; // 0x44
		case kVK_ANSI_KeypadPlus:     return MTY_SCANCODE_NP_PLUS; // 0x45
		case 0x46:                    return MTY_SCANCODE_NONE; // 0x46
		case kVK_ANSI_KeypadClear:    return MTY_SCANCODE_NUM_LOCK; // 0x47
		case kVK_VolumeUp:            return MTY_SCANCODE_VOLUME_UP; // 0x48
		case kVK_VolumeDown:          return MTY_SCANCODE_VOLUME_DOWN; // 0x49
		case kVK_Mute:                return MTY_SCANCODE_MUTE; // 0x4A
		case kVK_ANSI_KeypadDivide:   return MTY_SCANCODE_NP_DIVIDE; // 0x4B
		case kVK_ANSI_KeypadEnter:    return MTY_SCANCODE_NP_ENTER; // 0x4C
		case kVK_ANSI_KeypadMinus:    return MTY_SCANCODE_NP_MINUS; // 0x4E
		case kVK_F18:                 return MTY_SCANCODE_F18; // 0x4F
		case kVK_F19:                 return MTY_SCANCODE_F19; // 0x50
		case kVK_ANSI_KeypadEquals:   return MTY_SCANCODE_EQUALS; // 0x51
		case kVK_ANSI_Keypad0:        return MTY_SCANCODE_NP_0; // 0x52
		case kVK_ANSI_Keypad1:        return MTY_SCANCODE_NP_1; // 0x53
		case kVK_ANSI_Keypad2:        return MTY_SCANCODE_NP_2; // 0x54
		case kVK_ANSI_Keypad3:        return MTY_SCANCODE_NP_3; // 0x55
		case kVK_ANSI_Keypad4:        return MTY_SCANCODE_NP_4; // 0x56
		case kVK_ANSI_Keypad5:        return MTY_SCANCODE_NP_5; // 0x57
		case kVK_ANSI_Keypad6:        return MTY_SCANCODE_NP_6; // 0x58
		case kVK_ANSI_Keypad7:        return MTY_SCANCODE_NP_7; // 0x59
		case kVK_F20:                 return MTY_SCANCODE_NONE; // 0x5A
		case kVK_ANSI_Keypad8:        return MTY_SCANCODE_NP_8; // 0x5B
		case kVK_ANSI_Keypad9:        return MTY_SCANCODE_NP_9; // 0x5C
		case kVK_JIS_Yen:             return MTY_SCANCODE_YEN; // 0x5D
		case kVK_JIS_Underscore:      return MTY_SCANCODE_RO; // 0x5E
		case kVK_JIS_KeypadComma:     return MTY_SCANCODE_INTL_COMMA; // 0x5F
		case kVK_F5:                  return MTY_SCANCODE_F5; // 0x60
		case kVK_F6:                  return MTY_SCANCODE_F6; // 0x61
		case kVK_F7:                  return MTY_SCANCODE_F7; // 0x62
		case kVK_F3:                  return MTY_SCANCODE_F3; // 0x63
		case kVK_F8:                  return MTY_SCANCODE_F8; // 0x64
		case kVK_F9:                  return MTY_SCANCODE_F9; // 0x65
		case kVK_JIS_Eisu:            return MTY_SCANCODE_MUHENKAN; // 0x66
		case kVK_F11:                 return MTY_SCANCODE_F11; // 0x67
		case kVK_JIS_Kana:            return MTY_SCANCODE_HENKAN; // 0x68
		case kVK_F13:                 return MTY_SCANCODE_PRINT_SCREEN; // 0x69
		case kVK_F16:                 return MTY_SCANCODE_F16; // 0x6A
		case kVK_F14:                 return MTY_SCANCODE_SCROLL_LOCK; // 0x6B
		case kVK_F10:                 return MTY_SCANCODE_F10; // 0x6D
		case kVK_WinApp:              return MTY_SCANCODE_APP; // 0x6E
		case kVK_F12:                 return MTY_SCANCODE_F12; // 0x6F
		case 0x70:                    return MTY_SCANCODE_NONE; // 0x70
		case kVK_F15:                 return MTY_SCANCODE_PAUSE; // 0x71
		case kVK_Help:                return MTY_SCANCODE_INSERT; // 0x72
		case kVK_Home:                return MTY_SCANCODE_HOME; // 0x73
		case kVK_PageUp:              return MTY_SCANCODE_PAGE_UP; // 0x74
		case kVK_ForwardDelete:       return MTY_SCANCODE_DELETE; // 0x75
		case kVK_F4:                  return MTY_SCANCODE_F4; // 0x76
		case kVK_End:                 return MTY_SCANCODE_END; // 0x77
		case kVK_F2:                  return MTY_SCANCODE_F2; // 0x78
		case kVK_PageDown:            return MTY_SCANCODE_PAGE_DOWN; // 0x79
		case kVK_F1:                  return MTY_SCANCODE_F1; // 0x7A
		case kVK_LeftArrow:           return MTY_SCANCODE_LEFT; // 0x7B
		case kVK_RightArrow:          return MTY_SCANCODE_RIGHT; // 0x7C
		case kVK_DownArrow:           return MTY_SCANCODE_DOWN; // 0x7D
		case kVK_UpArrow:             return MTY_SCANCODE_UP; // 0x7E
	}

	return MTY_SCANCODE_NONE;
}

static const char *window_keycode_to_text(uint16_t kc)
{
	switch (kc) {
		case kVK_Return:              return "Enter"; // 0x24
		case kVK_Tab:                 return "Tab"; // 0x30
		case kVK_Space:               return "Space"; // 0x31
		case kVK_Delete:              return "Backspace"; // 0x33
		case kVK_Escape:              return "Esc"; // 0x35
		case kVK_RightCommand:        return "Command"; // 0x36
		case kVK_Command:             return "Command"; // 0x37
		case kVK_Shift:               return "Shift"; // 0x38
		case kVK_CapsLock:            return "Caps"; // 0x39
		case kVK_Option:              return "Alt"; // 0x3A
		case kVK_Control:             return "Alt"; // 0x3B
		case kVK_RightShift:          return "Shift"; // 0x3C
		case kVK_RightOption:         return "Alt"; // 0x3D
		case kVK_RightControl:        return "Ctrl"; // 0x3E
		case kVK_VolumeUp:            return "Volume Up"; // 0x48
		case kVK_VolumeDown:          return "Volume Down"; // 0x49
		case kVK_Mute:                return "Mute"; // 0x4A
		case kVK_F18:                 return "F18"; // 0x4F
		case kVK_F19:                 return "F19"; // 0x50
		case kVK_F20:                 return "F20"; // 0x5A
		case kVK_F5:                  return "F5"; // 0x60
		case kVK_F6:                  return "F6"; // 0x61
		case kVK_F7:                  return "F7"; // 0x62
		case kVK_F3:                  return "F3"; // 0x63
		case kVK_F8:                  return "F8"; // 0x64
		case kVK_F9:                  return "F9"; // 0x65
		case kVK_F11:                 return "F11"; // 0x67
		case kVK_F13:                 return "Print Screen"; // 0x69
		case kVK_F16:                 return "F16"; // 0x6A
		case kVK_F14:                 return "F14"; // 0x6B
		case kVK_F10:                 return "F10"; // 0x6D
		case kVK_WinApp:              return "App"; // 0x6E
		case kVK_F12:                 return "F12"; // 0x6F
		case kVK_F15:                 return "F15"; // 0x71
		case kVK_Help:                return "Insert"; // 0x72
		case kVK_Home:                return "Home"; // 0x73
		case kVK_PageUp:              return "Page Up"; // 0x74
		case kVK_ForwardDelete:       return "Delete"; // 0x75
		case kVK_F4:                  return "F4"; // 0x76
		case kVK_End:                 return "End"; // 0x77
		case kVK_F2:                  return "F2"; // 0x78
		case kVK_PageDown:            return "Page Down"; // 0x79
		case kVK_F1:                  return "F1"; // 0x7A
		case kVK_LeftArrow:           return "Left"; // 0x7B
		case kVK_RightArrow:          return "Right"; // 0x7C
		case kVK_DownArrow:           return "Down"; // 0x7D
		case kVK_UpArrow:             return "Up"; // 0x7E
	}

	return NULL;
}
