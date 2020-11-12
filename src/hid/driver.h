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


// Drivers

#include "ps4.h"
#include "nx.h"

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
		//case MTY_HID_DRIVER_DEFAULT:
		//	hid_default_state(hid, data, dsize, wmsg);
		//	break;
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
	}
}
