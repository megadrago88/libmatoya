// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "hid.h"


// Helpers

static void hid_u_to_s16(MTY_Value *v, bool invert)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT16_MAX);
	v->data = (int16_t) (invert ? -(d - INT16_MAX) : (d - INT16_MAX - 1));
	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

static void hid_s_to_s16(MTY_Value *v)
{
	if (v->data < 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->min * (float) INT16_MIN);

	if (v->data > 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->max * (float) INT16_MAX);

	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

static void hid_u_to_u8(MTY_Value *v)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT8_MAX);
	v->data = (int16_t) d;
	v->min = 0;
	v->max = UINT8_MAX;
}


// Default mapping

static bool hid_default_swap_value(MTY_Value *values, MTY_CValue a, MTY_CValue b)
{
	if (a != b && values[a].usage != values[b].usage) {
		MTY_Value tmp = values[b];
		values[b] = values[a];
		values[a] = tmp;
		return true;
	}

	return false;
}

static void hid_default_move_value(MTY_Controller *c, MTY_Value *v, uint16_t usage,
	int16_t data, int16_t min, int16_t max)
{
	if (v->usage != usage && c->numValues < MTY_CVALUE_MAX) {
		if (v->usage != 0x00) {
			MTY_Value *end = &c->values[c->numValues++];
			*end = *v;
		}

		v->usage = usage;
		v->data = data;
		v->min = min;
		v->max = max;

		// Fill in trigger values if they do not exist
		if (v->usage == 0x33)
			v->data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;

		if (v->usage == 0x34)
			v->data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	}
}

static void hid_default_map_values(MTY_Controller *c)
{
	// Make sure there is enough room for the standard CVALUEs
	if (c->numValues < MTY_CVALUE_DPAD + 1)
		c->numValues = MTY_CVALUE_DPAD + 1;

	// Swap positions
	for (uint8_t x = 0; x < c->numValues; x++) {
		retry:

		switch (c->values[x].usage) {
			case 0x30: // X -> Left Stick X
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_LX))
					goto retry;
				break;
			case 0x31: // Y -> Left Stick Y
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_LY))
					goto retry;
				break;
			case 0x32: // Z -> Right Stick X
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_RX))
					goto retry;
				break;
			case 0x35: // Rz -> Right Stick Y
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_RY))
					goto retry;
				break;
			case 0x33: // Rx -> Left Trigger
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_TRIGGER_L))
					goto retry;
				break;
			case 0x34: // Ry -> Right Trigger
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_TRIGGER_R))
					goto retry;
				break;
			case 0x39: // Hat -> DPAD
				if (hid_default_swap_value(c->values, x, MTY_CVALUE_DPAD))
					goto retry;
				break;
		}
	}

	// Move values that are not in the right positions to the end
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (x) {
			case MTY_CVALUE_THUMB_LX:  hid_default_move_value(c, v, 0x30, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_LY:  hid_default_move_value(c, v, 0x31, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RX:  hid_default_move_value(c, v, 0x32, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RY:  hid_default_move_value(c, v, 0x35, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_TRIGGER_L: hid_default_move_value(c, v, 0x33, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_TRIGGER_R: hid_default_move_value(c, v, 0x34, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_DPAD:      hid_default_move_value(c, v, 0x39, 8, 0, 7); break;
		}
	}

	// Convert to int16_t
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (v->usage) {
			case 0x30: // X -> Left Stick X
			case 0x32: // Z -> Right Stick X
				hid_u_to_s16(v, false);
				break;
			case 0x31: // Y -> Left Stick Y
			case 0x35: // Rz -> Right Stick Y
				hid_u_to_s16(v, true);
				break;
			case 0x33: // Rx -> Left Trigger
			case 0x34: // Ry -> Right Trigger
			case 0x36: // Slider
			case 0x37: // Dial
			case 0x38: // Wheel
				hid_u_to_u8(v);
				break;
		}
	}
}


// Drivers

#include "ps4.h"
#include "nx.h"
#include "xbox.h"

static MTY_HIDDriver hid_driver(struct hdevice *device)
{
	uint16_t vid = hid_device_get_vid(device);
	uint16_t pid = hid_device_get_pid(device);

	uint32_t id = ((uint32_t) vid << 16) | pid;

	switch (id) {
		// Switch
		case 0x057E2009: // Nintendo Switch Pro
		case 0x057E2006: // Nintendo Switch Joycon
		case 0x057E2007: // Nintendo Switch Joycon
		case 0x057E2017: // Nintendo Switch SNES Controller
		// case 0x0F0D00C1: // HORIPAD for Nintendo Switch
		// case 0x0F0D0092: // HORI Pokken Tournament DX Pro Pad
		case 0x0F0D00F6: // HORI Wireless Switch Pad
		// case 0x0E6F0185: // PDP Wired Fight Pad Pro for Nintendo Switch
		// case 0x0E6F0180: // PDP Faceoff Wired Pro Controller for Nintendo Switch
		// case 0x0E6F0181: // PDP Faceoff Deluxe Wired Pro Controller for Nintendo Switch
		// case 0x20D6A711: // PowerA Wired Controller Plus/PowerA Wired Controller Nintendo GameCube Style
		// case 0x20D6A712: // PowerA - Fusion Fight Pad
		// case 0x20D6A713: // PowerA - Super Mario Controller
			return MTY_HID_DRIVER_SWITCH;

		// PS4
		case 0x054C05C4: // Sony DualShock 4 Gen1
		case 0x054C09CC: // Sony DualShock 4 Gen2
		case 0x054C0BA0: // Sony PS4 Controller (Wireless dongle)
		case 0x0079181B: // Venom Arcade Stick - XXX:this may not work and may need to be called a ps3 controller
		case 0x054C05C5: // STRIKEPAD PS4 Grip Add-on
		case 0x07388250: // Mad Catz FightPad Pro PS4
		case 0x07388384: // Mad Catz FightStick TE S+ PS4
		case 0x07388480: // Mad Catz FightStick TE 2 PS4
		case 0x07388481: // Mad Catz FightStick TE 2+ PS4
		case 0x0C120E10: // Armor Armor 3 Pad PS4
		case 0x0C121CF6: // EMIO PS4 Elite Controller
		case 0x0C120E15: // Game:Pad 4
		case 0x0C120EF6: // Hitbox Arcade Stick
		case 0x0F0D0055: // HORIPAD 4 FPS
		case 0x0F0D0066: // HORIPAD 4 FPS Plus
		case 0x0F0D0084: // HORI Fighting Commander PS4
		case 0x0F0D008A: // HORI Real Arcade Pro 4
		case 0x0F0D009C: // HORI TAC PRO mousething
		case 0x0F0D00A0: // HORI TAC4 mousething
		case 0x0F0D00EE: // Hori mini wired https://www.playstation.com/en-us/explore/accessories/gaming-controllers/mini-wired-gamepad/
		case 0x11C04001: // "PS4 Fun Controller" added from user log
		case 0x146B0D01: // Nacon Revolution Pro Controller - has gyro
		case 0x146B0D02: // Nacon Revolution Pro Controller v2 - has gyro
		case 0x146B0D10: // NACON Revolution Infinite - has gyro
		case 0x15320401: // Razer Panthera PS4 Controller
		case 0x15321000: // Razer Raiju PS4 Controller
		case 0x15321004: // Razer Raiju 2 Ultimate USB
		case 0x15321007: // Razer Raiju 2 Tournament edition USB
		case 0x15321008: // Razer Panthera Evo Fightstick
		case 0x15321009: // Razer Raiju 2 Ultimate BT
		case 0x1532100A: // Razer Raiju 2 Tournament edition BT
		case 0x15321100: // Razer RAION Fightpad - Trackpad, no gyro, lightbar hardcoded to green
		case 0x20D6792A: // PowerA - Fusion Fight Pad
		case 0x75450104: // Armor 3 or Level Up Cobra - At least one variant has gyro
		case 0x98860025: // Astro C40
		case 0x0E6F0207: // Victrix Pro Fightstick w/ Touchpad for PS4
			return MTY_HID_DRIVER_PS4;

		case 0x045E02D1: // Microsoft X-Box One pad
		case 0x045E02DD: // Microsoft X-Box One pad (Firmware 2015)
		case 0x045E02E0: // Microsoft X-Box One S pad (Bluetooth)
		case 0x045E02E3: // Microsoft X-Box One Elite pad
		case 0x045E02EA: // Microsoft X-Box One S pad
		case 0x045E02FD: // Microsoft X-Box One S pad (Bluetooth)
		case 0x045E02FF: // Microsoft X-Box One Elite pad
		case 0x045E0B00: // Microsoft X-Box One Elite Series 2 pad
		case 0x045E0B05: // Microsoft X-Box One Elite Series 2 pad (Bluetooth)
		case 0x045E0B13: // Microsoft X-Box Series X Bluetooth (MTY)
		case 0x07384A01: // Mad Catz FightStick TE 2
		case 0x0E6F0139: // PDP Afterglow Wired Controller for Xbox One
		case 0x0E6F013B: // PDP Face-Off Gamepad for Xbox One
		case 0x0E6F013A: // PDP Xbox One Controller (unlisted)
		case 0x0E6F0145: // PDP MK X Fight Pad for Xbox One
		case 0x0E6F0146: // PDP Rock Candy Wired Controller for Xbox One
		case 0x0E6F015B: // PDP Fallout 4 Vault Boy Wired Controller for Xbox One
		case 0x0E6F015C: // PDP @Play Wired Controller for Xbox One
		case 0x0E6F015D: // PDP Mirror's Edge Wired Controller for Xbox One
		case 0x0E6F015F: // PDP Metallic Wired Controller for Xbox One
		case 0x0E6F0160: // PDP NFL Official Face-Off Wired Controller for Xbox One
		case 0x0E6F0161: // PDP Camo Wired Controller for Xbox One
		case 0x0E6F0162: // PDP Wired Controller for Xbox One
		case 0x0E6F0163: // PDP Legendary Collection: Deliverer of Truth
		case 0x0E6F0164: // PDP Battlefield 1 Official Wired Controller for Xbox One
		case 0x0E6F0165: // PDP Titanfall 2 Official Wired Controller for Xbox One
		case 0x0E6F0166: // PDP Mass Effect: Andromeda Official Wired Controller for Xbox One
		case 0x0E6F0167: // PDP Halo Wars 2 Official Face-Off Wired Controller for Xbox One
		case 0x0E6F0205: // PDP Victrix Pro Fight Stick
		case 0x0E6F0206: // PDP Mortal Kombat 25 Anniversary Edition Stick (Xbox One)
		case 0x0E6F0246: // PDP Rock Candy Wired Controller for Xbox One
		case 0x0E6F0261: // PDP Camo Wired Controller
		case 0x0E6F0262: // PDP Wired Controller
		case 0x0E6F02A0: // PDP Wired Controller for Xbox One - Midnight Blue
		case 0x0E6F02A1: // PDP Wired Controller for Xbox One - Verdant Green
		case 0x0E6F02A2: // PDP Wired Controller for Xbox One - Crimson Red
		case 0x0E6F02A3: // PDP Wired Controller for Xbox One - Arctic White
		case 0x0E6F02A4: // PDP Wired Controller for Xbox One - Stealth Series | Phantom Black
		case 0x0E6F02A5: // PDP Wired Controller for Xbox One - Stealth Series | Ghost White
		case 0x0E6F02A6: // PDP Wired Controller for Xbox One - Stealth Series | Revenant Blue
		case 0x0E6F02A7: // PDP Wired Controller for Xbox One - Raven Black
		case 0x0E6F02A8: // PDP Wired Controller for Xbox One - Arctic White
		case 0x0E6F02A9: // PDP Wired Controller for Xbox One - Midnight Blue
		case 0x0E6F02AA: // PDP Wired Controller for Xbox One - Verdant Green
		case 0x0E6F02AB: // PDP Wired Controller for Xbox One - Crimson Red
		case 0x0E6F02AC: // PDP Wired Controller for Xbox One - Ember Orange
		case 0x0E6F02AD: // PDP Wired Controller for Xbox One - Stealth Series | Phantom Black
		case 0x0E6F02AE: // PDP Wired Controller for Xbox One - Stealth Series | Ghost White
		case 0x0E6F02AF: // PDP Wired Controller for Xbox One - Stealth Series | Revenant Blue
		case 0x0E6F02B0: // PDP Wired Controller for Xbox One - Raven Black
		case 0x0E6F02B1: // PDP Wired Controller for Xbox One - Arctic White
		case 0x0E6F02B3: // PDP Afterglow Prismatic Wired Controller
		case 0x0E6F02B5: // PDP GAMEware Wired Controller Xbox One
		case 0x0E6F02B6: // PDP One-Handed Joystick Adaptive Controller
		case 0x0E6F02BD: // PDP Wired Controller for Xbox One - Royal Purple
		case 0x0E6F02BE: // PDP Deluxe Wired Controller for Xbox One - Raven Black
		case 0x0E6F02BF: // PDP Deluxe Wired Controller for Xbox One - Midnight Blue
		case 0x0E6F02C0: // PDP Deluxe Wired Controller for Xbox One - Stealth Series | Phantom Black
		case 0x0E6F02C1: // PDP Deluxe Wired Controller for Xbox One - Stealth Series | Ghost White
		case 0x0E6F02C2: // PDP Deluxe Wired Controller for Xbox One - Stealth Series | Revenant Blue
		case 0x0E6F02C3: // PDP Deluxe Wired Controller for Xbox One - Verdant Green
		case 0x0E6F02C4: // PDP Deluxe Wired Controller for Xbox One - Ember Orange
		case 0x0E6F02C5: // PDP Deluxe Wired Controller for Xbox One - Royal Purple
		case 0x0E6F02C6: // PDP Deluxe Wired Controller for Xbox One - Crimson Red
		case 0x0E6F02C7: // PDP Deluxe Wired Controller for Xbox One - Arctic White
		case 0x0E6F02C8: // PDP Kingdom Hearts Wired Controller
		case 0x0E6F02C9: // PDP Deluxe Wired Controller for Xbox One - Stealth Series | Phantasm Red
		case 0x0E6F02CA: // PDP Deluxe Wired Controller for Xbox One - Stealth Series | Specter Violet
		case 0x0E6F02CB: // PDP Wired Controller for Xbox One - Stealth Series | Specter Violet
		case 0x0E6F02CD: // PDP Rock Candy Wired Controller for Xbox One - Blu-merang
		case 0x0E6F02CE: // PDP Rock Candy Wired Controller for Xbox One - Cranblast
		case 0x0E6F02CF: // PDP Rock Candy Wired Controller for Xbox One - Aqualime
		case 0x0E6F02D5: // PDP Wired Controller for Xbox One - Red Camo
		case 0x0E6F0346: // PDP RC Gamepad for Xbox One
		case 0x0E6F0446: // PDP RC Gamepad for Xbox One
		case 0x0F0D0063: // Hori Real Arcade Pro Hayabusa (USA) Xbox One
		case 0x0F0D0067: // HORIPAD ONE
		case 0x0F0D0078: // Hori Real Arcade Pro V Kai Xbox One
		case 0x0F0D00C5: // HORI Fighting Commander
		case 0x15320A00: // Razer Atrox Arcade Stick
		case 0x15320A03: // Razer Wildcat
		case 0x24C6541A: // PowerA Xbox One Mini Wired Controller
		case 0x24C6542A: // Xbox ONE spectra
		case 0x24C6543A: // PowerA Xbox ONE liquid metal controller
		case 0x24C6551A: // PowerA FUSION Pro Controller
		case 0x24C6561A: // PowerA FUSION Controller
		case 0x24C6581A: // BDA XB1 Classic Controller
		case 0x24C6591A: // PowerA FUSION Pro Controller
		case 0x24C6592A: // BDA XB1 Spectra Pro
		case 0x24C6791A: // PowerA Fusion Fight Pad
		case 0x2E240652: // Hyperkin Duke
		case 0x2E241618: // Hyperkin Duke
		case 0x2E241688: // Hyperkin X91
		case 0x2F240050: // Unknown Controller
		case 0x2F24002E: // Unknown Controller
		case 0x98860024: // Unknown Controller
		case 0x2F240091: // Unknown Controller
		case 0x14300719: // Unknown Controller
		case 0x0F0D00ED: // Unknown Controller
		case 0x03EBFF02: // Unknown Controller
		case 0x0F0D00C0: // Unknown Controller
		case 0x0E6F0152: // Unknown Controller
		case 0x046D1007: // Unknown Controller
		case 0x0E6F02B8: // Unknown Controller
		case 0x2C222503: // Unknown Controller
		case 0x007918A1: // Unknown Controller
		case 0x00006686: // Unknown Controller
		case 0x11FF0511: // Unknown Controller
		case 0x12AB0304: // Unknown Controller
		case 0x14300291: // Unknown Controller
		case 0x143002A9: // Unknown Controller
		case 0x1430070B: // Unknown Controller
		case 0x146B0604: // Unknown Controller
		case 0x146B0605: // Unknown Controller
		case 0x146B0606: // Unknown Controller
		case 0x146B0609: // Unknown Controller
		case 0x15320A14: // Unknown Controller
		case 0x1BAD028E: // Unknown Controller
		case 0x1BAD02A0: // Unknown Controller
		case 0x1BAD5500: // Unknown Controller
		case 0x20AB55EF: // Unknown Controller
		case 0x24C65509: // Unknown Controller
		case 0x25160069: // Unknown Controller
		case 0x25B10360: // Unknown Controller
		case 0x2C222203: // Unknown Controller
		case 0x2F240011: // Unknown Controller
		case 0x2F240053: // Unknown Controller
		case 0x2F2400B7: // Unknown Controller
		case 0x046D0000: // Unknown Controller
		case 0x046D1004: // Unknown Controller
		case 0x046D1008: // Unknown Controller
		case 0x046DF301: // Unknown Controller
		case 0x073802A0: // Unknown Controller
		case 0x07387263: // Unknown Controller
		case 0x0738B738: // Unknown Controller
		case 0x0738CB29: // Unknown Controller
		case 0x0738F401: // Unknown Controller
		case 0x007918C2: // Unknown Controller
		case 0x007918C8: // Unknown Controller
		case 0x007918CF: // Unknown Controller
		case 0x0C120E17: // Unknown Controller
		case 0x0C120E1C: // Unknown Controller
		case 0x0C120E22: // Unknown Controller
		case 0x0C120E30: // Unknown Controller
		case 0xD2D2D2D2: // Unknown Controller
		case 0x0D629A1A: // Unknown Controller
		case 0x0D629A1B: // Unknown Controller
		case 0x0E000E00: // Unknown Controller
		case 0x0E6F012A: // Unknown Controller
		case 0x0E6F02B2: // Unknown Controller
		case 0x0F0D0097: // Unknown Controller
		case 0x0F0D00BA: // Unknown Controller
		case 0x0F0D00D8: // Unknown Controller
		case 0x0FFF02A1: // Unknown Controller
			return MTY_HID_DRIVER_XBOX;
	}

	return MTY_HID_DRIVER_DEFAULT;
}

static void hid_driver_init(struct hdevice *device)
{
	switch (hid_driver(device)) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_init(device);
			break;
		case MTY_HID_DRIVER_PS4:
			hid_ps4_init(device);
			break;
		case MTY_HID_DRIVER_XBOX:
			hid_xbox_init(device);
			break;
	}
}

static void hid_driver_state(struct hdevice *device, const void *buf, size_t size, MTY_Msg *wmsg)
{
	switch (hid_driver(device)) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_state(device, buf, size, wmsg);
			break;
		case MTY_HID_DRIVER_PS4:
			hid_ps4_state(device, buf, size, wmsg);
			break;
		case MTY_HID_DRIVER_XBOX:
			hid_xbox_state(device, buf, size, wmsg);
			break;
		case MTY_HID_DRIVER_DEFAULT:
			hid_default_state(device, buf, size, wmsg);
			hid_default_map_values(&wmsg->controller);
			break;
	}
}

static void hid_driver_rumble(struct hid *hid, uint32_t id, uint16_t low, uint16_t high)
{
	struct hdevice *device = hid_get_device_by_id(hid, id);
	if (!device)
		return;

	switch (hid_driver(device)) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_rumble(device, low > 0, high > 0);
			break;
		case MTY_HID_DRIVER_PS4:
			hid_ps4_rumble(device, low, high);
			break;
		case MTY_HID_DRIVER_XBOX:
			hid_xbox_rumble(device, low, high);
			break;
	}
}
