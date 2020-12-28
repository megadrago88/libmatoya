// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

static const KeySym APP_KEY_MAP[MTY_SCANCODE_MAX] = {
	[MTY_SCANCODE_NONE]           = 0,
	[MTY_SCANCODE_ESCAPE]         = XK_Escape,
	[MTY_SCANCODE_1]              = XK_1,
	[MTY_SCANCODE_2]              = XK_2,
	[MTY_SCANCODE_3]              = XK_3,
	[MTY_SCANCODE_4]              = XK_4,
	[MTY_SCANCODE_5]              = XK_5,
	[MTY_SCANCODE_6]              = XK_6,
	[MTY_SCANCODE_7]              = XK_7,
	[MTY_SCANCODE_8]              = XK_8,
	[MTY_SCANCODE_9]              = XK_9,
	[MTY_SCANCODE_0]              = XK_0,
	[MTY_SCANCODE_MINUS]          = XK_minus,
	[MTY_SCANCODE_EQUALS]         = XK_equal,
	[MTY_SCANCODE_BACKSPACE]      = XK_BackSpace,
	[MTY_SCANCODE_TAB]            = XK_Tab,
	[MTY_SCANCODE_Q]              = XK_q,
	[MTY_SCANCODE_AUDIO_PREV]     = 0,
	[MTY_SCANCODE_W]              = XK_w,
	[MTY_SCANCODE_E]              = XK_e,
	[MTY_SCANCODE_R]              = XK_r,
	[MTY_SCANCODE_T]              = XK_t,
	[MTY_SCANCODE_Y]              = XK_y,
	[MTY_SCANCODE_U]              = XK_u,
	[MTY_SCANCODE_I]              = XK_i,
	[MTY_SCANCODE_O]              = XK_o,
	[MTY_SCANCODE_P]              = XK_p,
	[MTY_SCANCODE_AUDIO_NEXT]     = 0,
	[MTY_SCANCODE_LBRACKET]       = XK_bracketleft,
	[MTY_SCANCODE_RBRACKET]       = XK_bracketright,
	[MTY_SCANCODE_ENTER]          = XK_Return,
	[MTY_SCANCODE_NP_ENTER]       = XK_KP_Enter,
	[MTY_SCANCODE_LCTRL]          = XK_Control_L,
	[MTY_SCANCODE_RCTRL]          = XK_Control_R,
	[MTY_SCANCODE_A]              = XK_a,
	[MTY_SCANCODE_S]              = XK_s,
	[MTY_SCANCODE_D]              = XK_d,
	[MTY_SCANCODE_MUTE]           = 0,
	[MTY_SCANCODE_F]              = XK_f,
	[MTY_SCANCODE_G]              = XK_g,
	[MTY_SCANCODE_AUDIO_PLAY]     = 0,
	[MTY_SCANCODE_H]              = XK_h,
	[MTY_SCANCODE_J]              = XK_j,
	[MTY_SCANCODE_AUDIO_STOP]     = 0,
	[MTY_SCANCODE_K]              = XK_k,
	[MTY_SCANCODE_L]              = XK_l,
	[MTY_SCANCODE_SEMICOLON]      = XK_semicolon,
	[MTY_SCANCODE_QUOTE]          = XK_apostrophe,
	[MTY_SCANCODE_GRAVE]          = XK_grave,
	[MTY_SCANCODE_LSHIFT]         = XK_Shift_L,
	[MTY_SCANCODE_BACKSLASH]      = XK_backslash,
	[MTY_SCANCODE_Z]              = XK_z,
	[MTY_SCANCODE_X]              = XK_x,
	[MTY_SCANCODE_C]              = XK_c,
	[MTY_SCANCODE_VOLUME_DOWN]    = 0,
	[MTY_SCANCODE_V]              = XK_v,
	[MTY_SCANCODE_B]              = XK_b,
	[MTY_SCANCODE_VOLUME_UP]      = 0,
	[MTY_SCANCODE_N]              = XK_n,
	[MTY_SCANCODE_M]              = XK_m,
	[MTY_SCANCODE_COMMA]          = XK_comma,
	[MTY_SCANCODE_PERIOD]         = XK_period,
	[MTY_SCANCODE_SLASH]          = XK_slash,
	[MTY_SCANCODE_NP_DIVIDE]      = XK_KP_Divide,
	[MTY_SCANCODE_RSHIFT]         = XK_Shift_R,
	[MTY_SCANCODE_NP_MULTIPLY]    = XK_KP_Multiply,
	[MTY_SCANCODE_PRINT_SCREEN]   = XK_Print,
	[MTY_SCANCODE_LALT]           = XK_Alt_L,
	[MTY_SCANCODE_RALT]           = XK_Alt_R,
	[MTY_SCANCODE_SPACE]          = XK_space,
	[MTY_SCANCODE_CAPS]           = XK_Caps_Lock,
	[MTY_SCANCODE_F1]             = XK_F1,
	[MTY_SCANCODE_F2]             = XK_F2,
	[MTY_SCANCODE_F3]             = XK_F3,
	[MTY_SCANCODE_F4]             = XK_F4,
	[MTY_SCANCODE_F5]             = XK_F5,
	[MTY_SCANCODE_F6]             = XK_F6,
	[MTY_SCANCODE_F7]             = XK_F7,
	[MTY_SCANCODE_F8]             = XK_F8,
	[MTY_SCANCODE_F9]             = XK_F9,
	[MTY_SCANCODE_F10]            = XK_F10,
	[MTY_SCANCODE_NUM_LOCK]       = XK_Num_Lock,
	[MTY_SCANCODE_SCROLL_LOCK]    = XK_Scroll_Lock,
	[MTY_SCANCODE_PAUSE]          = XK_Pause,
	[MTY_SCANCODE_NP_7]           = XK_KP_Home,
	[MTY_SCANCODE_HOME]           = XK_Home,
	[MTY_SCANCODE_NP_8]           = XK_KP_Up,
	[MTY_SCANCODE_UP]             = XK_Up,
	[MTY_SCANCODE_NP_9]           = XK_KP_Page_Up,
	[MTY_SCANCODE_PAGE_UP]        = XK_Page_Up,
	[MTY_SCANCODE_NP_MINUS]       = XK_KP_Subtract,
	[MTY_SCANCODE_NP_4]           = XK_KP_Left,
	[MTY_SCANCODE_LEFT]           = XK_Left,
	[MTY_SCANCODE_NP_5]           = XK_KP_Begin,
	[MTY_SCANCODE_NP_6]           = XK_KP_Right,
	[MTY_SCANCODE_RIGHT]          = XK_Right,
	[MTY_SCANCODE_NP_PLUS]        = XK_KP_Add,
	[MTY_SCANCODE_NP_1]           = XK_KP_End,
	[MTY_SCANCODE_END]            = XK_End,
	[MTY_SCANCODE_NP_2]           = XK_KP_Down,
	[MTY_SCANCODE_DOWN]           = XK_Down,
	[MTY_SCANCODE_NP_3]           = XK_KP_Page_Down,
	[MTY_SCANCODE_PAGE_DOWN]      = XK_Page_Down,
	[MTY_SCANCODE_NP_0]           = XK_KP_Insert,
	[MTY_SCANCODE_INSERT]         = XK_Insert,
	[MTY_SCANCODE_NP_PERIOD]      = XK_KP_Delete,
	[MTY_SCANCODE_DELETE]         = XK_Delete,
	[MTY_SCANCODE_INTL_BACKSLASH] = 0,
	[MTY_SCANCODE_F11]            = XK_F11,
	[MTY_SCANCODE_F12]            = XK_F12,
	[MTY_SCANCODE_LWIN]           = XK_Super_L,
	[MTY_SCANCODE_RWIN]           = XK_Super_R,
	[MTY_SCANCODE_APP]            = XK_Menu,
	[MTY_SCANCODE_F13]            = XK_F13,
	[MTY_SCANCODE_F14]            = XK_F14,
	[MTY_SCANCODE_F15]            = XK_F15,
	[MTY_SCANCODE_F16]            = XK_F16,
	[MTY_SCANCODE_F17]            = XK_F17,
	[MTY_SCANCODE_F18]            = XK_F18,
	[MTY_SCANCODE_F19]            = XK_F19,
	[MTY_SCANCODE_MEDIA_SELECT]   = 0,
	[MTY_SCANCODE_JP]             = 0,
	[MTY_SCANCODE_RO]             = XK_Romaji,
	[MTY_SCANCODE_HENKAN]         = XK_Henkan,
	[MTY_SCANCODE_MUHENKAN]       = XK_Muhenkan,
	[MTY_SCANCODE_INTL_COMMA]     = 0,
	[MTY_SCANCODE_YEN]            = XK_yen,
};

static MTY_Keymod window_keystate_to_keymod(KeySym key, bool pressed, unsigned int state)
{
	MTY_Keymod mod = MTY_KEYMOD_NONE;

	if (state & ShiftMask)    mod |= MTY_KEYMOD_LSHIFT;
	if (state & LockMask)     mod |= MTY_KEYMOD_CAPS;
	if (state & ControlMask)  mod |= MTY_KEYMOD_LCTRL;
	if (state & Mod1Mask)     mod |= MTY_KEYMOD_LALT;
	if (state & Mod2Mask)     mod |= MTY_KEYMOD_NUM;
	if (state & Mod4Mask)     mod |= MTY_KEYMOD_LWIN;

	// X11 gives you the mods just before the current key so we need to incorporate
	// the current event's keysym

	if (key == XK_Shift_L || key == XK_Shift_R)
		mod = pressed ? (mod | MTY_KEYMOD_LSHIFT) : (mod & ~MTY_KEYMOD_LSHIFT);

	if (key == XK_Control_L || key == XK_Control_R)
		mod = pressed ? (mod | MTY_KEYMOD_LCTRL) : (mod & ~MTY_KEYMOD_LCTRL);

	if (key == XK_Alt_L || key == XK_Alt_R)
		mod = pressed ? (mod | MTY_KEYMOD_LALT) : (mod & ~MTY_KEYMOD_LALT);

	if (key == XK_Super_L || key == XK_Super_R)
		mod = pressed ? (mod | MTY_KEYMOD_LWIN) : (mod & ~MTY_KEYMOD_LWIN);

	return mod;
}

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
