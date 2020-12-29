// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

static const KeySym APP_KEY_MAP[MTY_KEY_MAX] = {
	[MTY_KEY_NONE]           = 0,
	[MTY_KEY_ESCAPE]         = XK_Escape,
	[MTY_KEY_1]              = XK_1,
	[MTY_KEY_2]              = XK_2,
	[MTY_KEY_3]              = XK_3,
	[MTY_KEY_4]              = XK_4,
	[MTY_KEY_5]              = XK_5,
	[MTY_KEY_6]              = XK_6,
	[MTY_KEY_7]              = XK_7,
	[MTY_KEY_8]              = XK_8,
	[MTY_KEY_9]              = XK_9,
	[MTY_KEY_0]              = XK_0,
	[MTY_KEY_MINUS]          = XK_minus,
	[MTY_KEY_EQUALS]         = XK_equal,
	[MTY_KEY_BACKSPACE]      = XK_BackSpace,
	[MTY_KEY_TAB]            = XK_Tab,
	[MTY_KEY_Q]              = XK_q,
	[MTY_KEY_AUDIO_PREV]     = 0,
	[MTY_KEY_W]              = XK_w,
	[MTY_KEY_E]              = XK_e,
	[MTY_KEY_R]              = XK_r,
	[MTY_KEY_T]              = XK_t,
	[MTY_KEY_Y]              = XK_y,
	[MTY_KEY_U]              = XK_u,
	[MTY_KEY_I]              = XK_i,
	[MTY_KEY_O]              = XK_o,
	[MTY_KEY_P]              = XK_p,
	[MTY_KEY_AUDIO_NEXT]     = 0,
	[MTY_KEY_LBRACKET]       = XK_bracketleft,
	[MTY_KEY_RBRACKET]       = XK_bracketright,
	[MTY_KEY_ENTER]          = XK_Return,
	[MTY_KEY_NP_ENTER]       = XK_KP_Enter,
	[MTY_KEY_LCTRL]          = XK_Control_L,
	[MTY_KEY_RCTRL]          = XK_Control_R,
	[MTY_KEY_A]              = XK_a,
	[MTY_KEY_S]              = XK_s,
	[MTY_KEY_D]              = XK_d,
	[MTY_KEY_MUTE]           = 0,
	[MTY_KEY_F]              = XK_f,
	[MTY_KEY_G]              = XK_g,
	[MTY_KEY_AUDIO_PLAY]     = 0,
	[MTY_KEY_H]              = XK_h,
	[MTY_KEY_J]              = XK_j,
	[MTY_KEY_AUDIO_STOP]     = 0,
	[MTY_KEY_K]              = XK_k,
	[MTY_KEY_L]              = XK_l,
	[MTY_KEY_SEMICOLON]      = XK_semicolon,
	[MTY_KEY_QUOTE]          = XK_apostrophe,
	[MTY_KEY_GRAVE]          = XK_grave,
	[MTY_KEY_LSHIFT]         = XK_Shift_L,
	[MTY_KEY_BACKSLASH]      = XK_backslash,
	[MTY_KEY_Z]              = XK_z,
	[MTY_KEY_X]              = XK_x,
	[MTY_KEY_C]              = XK_c,
	[MTY_KEY_VOLUME_DOWN]    = 0,
	[MTY_KEY_V]              = XK_v,
	[MTY_KEY_B]              = XK_b,
	[MTY_KEY_VOLUME_UP]      = 0,
	[MTY_KEY_N]              = XK_n,
	[MTY_KEY_M]              = XK_m,
	[MTY_KEY_COMMA]          = XK_comma,
	[MTY_KEY_PERIOD]         = XK_period,
	[MTY_KEY_SLASH]          = XK_slash,
	[MTY_KEY_NP_DIVIDE]      = XK_KP_Divide,
	[MTY_KEY_RSHIFT]         = XK_Shift_R,
	[MTY_KEY_NP_MULTIPLY]    = XK_KP_Multiply,
	[MTY_KEY_PRINT_SCREEN]   = XK_Print,
	[MTY_KEY_LALT]           = XK_Alt_L,
	[MTY_KEY_RALT]           = XK_Alt_R,
	[MTY_KEY_SPACE]          = XK_space,
	[MTY_KEY_CAPS]           = XK_Caps_Lock,
	[MTY_KEY_F1]             = XK_F1,
	[MTY_KEY_F2]             = XK_F2,
	[MTY_KEY_F3]             = XK_F3,
	[MTY_KEY_F4]             = XK_F4,
	[MTY_KEY_F5]             = XK_F5,
	[MTY_KEY_F6]             = XK_F6,
	[MTY_KEY_F7]             = XK_F7,
	[MTY_KEY_F8]             = XK_F8,
	[MTY_KEY_F9]             = XK_F9,
	[MTY_KEY_F10]            = XK_F10,
	[MTY_KEY_NUM_LOCK]       = XK_Num_Lock,
	[MTY_KEY_SCROLL_LOCK]    = XK_Scroll_Lock,
	[MTY_KEY_PAUSE]          = XK_Pause,
	[MTY_KEY_NP_7]           = XK_KP_Home,
	[MTY_KEY_HOME]           = XK_Home,
	[MTY_KEY_NP_8]           = XK_KP_Up,
	[MTY_KEY_UP]             = XK_Up,
	[MTY_KEY_NP_9]           = XK_KP_Page_Up,
	[MTY_KEY_PAGE_UP]        = XK_Page_Up,
	[MTY_KEY_NP_MINUS]       = XK_KP_Subtract,
	[MTY_KEY_NP_4]           = XK_KP_Left,
	[MTY_KEY_LEFT]           = XK_Left,
	[MTY_KEY_NP_5]           = XK_KP_Begin,
	[MTY_KEY_NP_6]           = XK_KP_Right,
	[MTY_KEY_RIGHT]          = XK_Right,
	[MTY_KEY_NP_PLUS]        = XK_KP_Add,
	[MTY_KEY_NP_1]           = XK_KP_End,
	[MTY_KEY_END]            = XK_End,
	[MTY_KEY_NP_2]           = XK_KP_Down,
	[MTY_KEY_DOWN]           = XK_Down,
	[MTY_KEY_NP_3]           = XK_KP_Page_Down,
	[MTY_KEY_PAGE_DOWN]      = XK_Page_Down,
	[MTY_KEY_NP_0]           = XK_KP_Insert,
	[MTY_KEY_INSERT]         = XK_Insert,
	[MTY_KEY_NP_PERIOD]      = XK_KP_Delete,
	[MTY_KEY_DELETE]         = XK_Delete,
	[MTY_KEY_INTL_BACKSLASH] = 0,
	[MTY_KEY_F11]            = XK_F11,
	[MTY_KEY_F12]            = XK_F12,
	[MTY_KEY_LWIN]           = XK_Super_L,
	[MTY_KEY_RWIN]           = XK_Super_R,
	[MTY_KEY_APP]            = XK_Menu,
	[MTY_KEY_F13]            = XK_F13,
	[MTY_KEY_F14]            = XK_F14,
	[MTY_KEY_F15]            = XK_F15,
	[MTY_KEY_F16]            = XK_F16,
	[MTY_KEY_F17]            = XK_F17,
	[MTY_KEY_F18]            = XK_F18,
	[MTY_KEY_F19]            = XK_F19,
	[MTY_KEY_MEDIA_SELECT]   = 0,
	[MTY_KEY_JP]             = 0,
	[MTY_KEY_RO]             = XK_Romaji,
	[MTY_KEY_HENKAN]         = XK_Henkan,
	[MTY_KEY_MUHENKAN]       = XK_Muhenkan,
	[MTY_KEY_INTL_COMMA]     = 0,
	[MTY_KEY_YEN]            = XK_yen,
};

static MTY_Mod window_keystate_to_keymod(KeySym key, bool pressed, unsigned int state)
{
	MTY_Mod mod = MTY_MOD_NONE;

	if (state & ShiftMask)    mod |= MTY_MOD_LSHIFT;
	if (state & LockMask)     mod |= MTY_MOD_CAPS;
	if (state & ControlMask)  mod |= MTY_MOD_LCTRL;
	if (state & Mod1Mask)     mod |= MTY_MOD_LALT;
	if (state & Mod2Mask)     mod |= MTY_MOD_NUM;
	if (state & Mod4Mask)     mod |= MTY_MOD_LWIN;

	// X11 gives you the mods just before the current key so we need to incorporate
	// the current event's keysym

	if (key == XK_Shift_L || key == XK_Shift_R)
		mod = pressed ? (mod | MTY_MOD_LSHIFT) : (mod & ~MTY_MOD_LSHIFT);

	if (key == XK_Control_L || key == XK_Control_R)
		mod = pressed ? (mod | MTY_MOD_LCTRL) : (mod & ~MTY_MOD_LCTRL);

	if (key == XK_Alt_L || key == XK_Alt_R)
		mod = pressed ? (mod | MTY_MOD_LALT) : (mod & ~MTY_MOD_LALT);

	if (key == XK_Super_L || key == XK_Super_R)
		mod = pressed ? (mod | MTY_MOD_LWIN) : (mod & ~MTY_MOD_LWIN);

	return mod;
}

static MTY_Key window_keysym_to_scancode(KeySym sym)
{
	switch (sym) {
		case XK_a:             return MTY_KEY_A;
		case XK_s:             return MTY_KEY_S;
		case XK_d:             return MTY_KEY_D;
		case XK_f:             return MTY_KEY_F;
		case XK_h:             return MTY_KEY_H;
		case XK_g:             return MTY_KEY_G;
		case XK_z:             return MTY_KEY_Z;
		case XK_x:             return MTY_KEY_X;
		case XK_c:             return MTY_KEY_C;
		case XK_v:             return MTY_KEY_V;
		// case XK_ISO_Section:  return MTY_KEY_INTL_BACKSLASH;
		case XK_b:             return MTY_KEY_B;
		case XK_q:             return MTY_KEY_Q;
		case XK_w:             return MTY_KEY_W;
		case XK_e:             return MTY_KEY_E;
		case XK_r:             return MTY_KEY_R;
		case XK_y:             return MTY_KEY_Y;
		case XK_t:             return MTY_KEY_T;
		case XK_1:             return MTY_KEY_1;
		case XK_2:             return MTY_KEY_2;
		case XK_3:             return MTY_KEY_3;
		case XK_4:             return MTY_KEY_4;
		case XK_6:             return MTY_KEY_6;
		case XK_5:             return MTY_KEY_5;
		case XK_equal:         return MTY_KEY_EQUALS;
		case XK_9:             return MTY_KEY_9;
		case XK_7:             return MTY_KEY_7;
		case XK_minus:         return MTY_KEY_MINUS;
		case XK_8:             return MTY_KEY_8;
		case XK_0:             return MTY_KEY_0;
		case XK_bracketright:  return MTY_KEY_RBRACKET;
		case XK_o:             return MTY_KEY_O;
		case XK_u:             return MTY_KEY_U;
		case XK_bracketleft:   return MTY_KEY_LBRACKET;
		case XK_i:             return MTY_KEY_I;
		case XK_p:             return MTY_KEY_P;
		case XK_Return:        return MTY_KEY_ENTER;
		case XK_l:             return MTY_KEY_L;
		case XK_j:             return MTY_KEY_J;
		case XK_apostrophe:    return MTY_KEY_QUOTE;
		case XK_k:             return MTY_KEY_K;
		case XK_semicolon:     return MTY_KEY_SEMICOLON;
		case XK_backslash:     return MTY_KEY_BACKSLASH;
		case XK_comma:         return MTY_KEY_COMMA;
		case XK_slash:         return MTY_KEY_SLASH;
		case XK_n:             return MTY_KEY_N;
		case XK_m:             return MTY_KEY_M;
		case XK_period:        return MTY_KEY_PERIOD;
		case XK_Tab:           return MTY_KEY_TAB;
		case XK_space:         return MTY_KEY_SPACE;
		case XK_grave:         return MTY_KEY_GRAVE;
		case XK_BackSpace:     return MTY_KEY_BACKSPACE;
		case XK_Escape:        return MTY_KEY_ESCAPE;
		case XK_Super_R:       return MTY_KEY_RWIN;
		case XK_Super_L:       return MTY_KEY_LWIN;
		case XK_Shift_L:       return MTY_KEY_LSHIFT;
		case XK_Caps_Lock:     return MTY_KEY_CAPS;
		case XK_Alt_L:         return MTY_KEY_LALT;
		case XK_Control_L:     return MTY_KEY_LCTRL;
		case XK_Shift_R:       return MTY_KEY_RSHIFT;
		case XK_Alt_R:         return MTY_KEY_RALT;
		case XK_Control_R:     return MTY_KEY_RCTRL;
		case XK_F17:           return MTY_KEY_F17;
		case XK_KP_Delete:     return MTY_KEY_NP_PERIOD;
		case XK_KP_Multiply:   return MTY_KEY_NP_MULTIPLY;
		case XK_KP_Add:        return MTY_KEY_NP_PLUS;
		case XK_Num_Lock:      return MTY_KEY_NUM_LOCK;
		// case XK_VolumeUp:     return MTY_KEY_VOLUME_UP;
		// case XK_VolumeDown:   return MTY_KEY_VOLUME_DOWN;
		// case XK_Mute:         return MTY_KEY_MUTE;
		case XK_KP_Divide:     return MTY_KEY_NP_DIVIDE;
		case XK_KP_Enter:      return MTY_KEY_NP_ENTER;
		case XK_KP_Subtract:   return MTY_KEY_NP_MINUS;
		case XK_F18:           return MTY_KEY_F18;
		case XK_F19:           return MTY_KEY_F19;
		case XK_KP_Insert:     return MTY_KEY_NP_0;
		case XK_KP_End:        return MTY_KEY_NP_1;
		case XK_KP_Down:       return MTY_KEY_NP_2;
		case XK_KP_Page_Down:  return MTY_KEY_NP_3;
		case XK_KP_Left:       return MTY_KEY_NP_4;
		case XK_KP_Begin:      return MTY_KEY_NP_5;
		case XK_KP_Right:      return MTY_KEY_NP_6;
		case XK_KP_Home:       return MTY_KEY_NP_7;
		case XK_KP_Up:         return MTY_KEY_NP_8;
		case XK_KP_Page_Up:    return MTY_KEY_NP_9;
		case XK_yen:           return MTY_KEY_YEN;
		case XK_Romaji:        return MTY_KEY_RO;
		// case XK_KeypadComma:  return MTY_KEY_INTL_COMMA;
		case XK_F5:            return MTY_KEY_F5;
		case XK_F6:            return MTY_KEY_F6;
		case XK_F7:            return MTY_KEY_F7;
		case XK_F3:            return MTY_KEY_F3;
		case XK_F8:            return MTY_KEY_F8;
		case XK_F9:            return MTY_KEY_F9;
		case XK_Muhenkan:      return MTY_KEY_MUHENKAN;
		case XK_F11:           return MTY_KEY_F11;
		case XK_Henkan:        return MTY_KEY_HENKAN;
		case XK_F13:           return MTY_KEY_F13;
		case XK_F16:           return MTY_KEY_F16;
		case XK_F14:           return MTY_KEY_F14;
		case XK_F10:           return MTY_KEY_F10;
		case XK_Menu:          return MTY_KEY_APP;
		case XK_F12:           return MTY_KEY_F12;
		case XK_F15:           return MTY_KEY_F15;
		case XK_Insert:        return MTY_KEY_INSERT;
		case XK_Home:          return MTY_KEY_HOME;
		case XK_Page_Up:       return MTY_KEY_PAGE_UP;
		case XK_Delete:        return MTY_KEY_DELETE;
		case XK_F4:            return MTY_KEY_F4;
		case XK_End:           return MTY_KEY_END;
		case XK_F2:            return MTY_KEY_F2;
		case XK_Page_Down:     return MTY_KEY_PAGE_DOWN;
		case XK_F1:            return MTY_KEY_F1;
		case XK_Left:          return MTY_KEY_LEFT;
		case XK_Right:         return MTY_KEY_RIGHT;
		case XK_Down:          return MTY_KEY_DOWN;
		case XK_Up:            return MTY_KEY_UP;
		case XK_Print:         return MTY_KEY_PRINT_SCREEN;
		case XK_Scroll_Lock:   return MTY_KEY_SCROLL_LOCK;
		case XK_Pause:         return MTY_KEY_PAUSE;
	}

	return MTY_KEY_NONE;
}
