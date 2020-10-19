// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define HID_SWITCH_OUTPUT_MAX 64

static void hid_switch_init(struct hid *hid)
{
	uint8_t reply[HID_SWITCH_OUTPUT_MAX] = {0x80, 0x02};
	hid_write(hid->name, reply, HID_SWITCH_OUTPUT_MAX);
}

static void hid_switch_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	const uint8_t *d = data;

	// Switch Handshake
	if (d[0] == 0x81 && d[1] == 0x02 && d[2] == 0x00 && d[3] == 0x00) {
		uint8_t reply[HID_SWITCH_OUTPUT_MAX] = {0x80, 0x04};
		hid_write(hid->name, reply, HID_SWITCH_OUTPUT_MAX);

	// State (Full)
	} else if (d[0] == 0x30) {
		wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = (uint16_t) hid->di.hid.dwVendorId;
		c->pid = (uint16_t) hid->di.hid.dwProductId;
		c->numValues = 7;
		c->numButtons = 14;
		c->driver = hid->driver;
		c->id = hid->id;

		c->buttons[MTY_CBUTTON_X] = d[3] & 0x01;
		c->buttons[MTY_CBUTTON_A] = d[3] & 0x04;
		c->buttons[MTY_CBUTTON_B] = d[3] & 0x08;
		c->buttons[MTY_CBUTTON_Y] = d[3] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[5] & 0x40;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[3] & 0x40;
		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[5] & 0x80;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[3] & 0x80;
		c->buttons[MTY_CBUTTON_BACK] = d[4] & 0x01;
		c->buttons[MTY_CBUTTON_START] = d[4] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[4] & 0x08;
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[4] & 0x04;
		c->buttons[MTY_CBUTTON_GUIDE] = d[4] & 0x10;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = d[4] & 0x20; // The "Capture" button

		uint16_t lx = d[6] | ((d[7] & 0x0F) << 8);
		uint16_t ly = (d[7] >> 4) | (d[8] << 4);
		uint16_t rx = d[9] | ((d[10] & 0x0F) << 8);
		uint16_t ry = (d[10] >> 4) | (d[11] << 4);

		c->values[MTY_CVALUE_THUMB_LX].data = lx;
		c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
		c->values[MTY_CVALUE_THUMB_LX].min = 300;
		c->values[MTY_CVALUE_THUMB_LX].max = 3800;

		c->values[MTY_CVALUE_THUMB_LY].data = ly;
		c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
		c->values[MTY_CVALUE_THUMB_LY].min = 300;
		c->values[MTY_CVALUE_THUMB_LY].max = 3800;

		c->values[MTY_CVALUE_THUMB_RX].data = rx;
		c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
		c->values[MTY_CVALUE_THUMB_RX].min = 300;
		c->values[MTY_CVALUE_THUMB_RX].max = 3800;

		c->values[MTY_CVALUE_THUMB_RY].data = ry;
		c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
		c->values[MTY_CVALUE_THUMB_RY].min = 300;
		c->values[MTY_CVALUE_THUMB_RY].max = 3800;

		c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		c->values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
		c->values[MTY_CVALUE_TRIGGER_L].min = 0;
		c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

		c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		c->values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
		c->values[MTY_CVALUE_TRIGGER_R].min = 0;
		c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

		bool up = d[5] & 0x02;
		bool down = d[5] & 0x01;
		bool left = d[5] & 0x08;
		bool right = d[5] & 0x04;

		c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
			(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

	// State (Simple)
	} else if (d[0] == 0x3F) {
		wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = (uint16_t) hid->di.hid.dwVendorId;
		c->pid = (uint16_t) hid->di.hid.dwProductId;
		c->numValues = 7;
		c->numButtons = 14;
		c->driver = hid->driver;
		c->id = hid->id;

		c->buttons[MTY_CBUTTON_X] = d[1] & 0x01;
		c->buttons[MTY_CBUTTON_A] = d[1] & 0x04;
		c->buttons[MTY_CBUTTON_B] = d[1] & 0x08;
		c->buttons[MTY_CBUTTON_Y] = d[1] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[1] & 0x10;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[1] & 0x20;
		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = false;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = false;
		c->buttons[MTY_CBUTTON_BACK] = (d[2] & 0x01) || (d[2] & 0x10);  // Minus || Home
		c->buttons[MTY_CBUTTON_START] = (d[2] & 0x02) || (d[2] & 0x20); // Home || Capture
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = (d[2] & 0x04) || (d[2] & 0x08); // Left Stick || Right Stick
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = false;
		c->buttons[MTY_CBUTTON_GUIDE] = false;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = false;

		c->values[MTY_CVALUE_DPAD].data = d[3];
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;
	}
}
