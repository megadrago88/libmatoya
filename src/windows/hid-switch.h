// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

static void hid_switch_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	const uint8_t *d = data;

	// Switch Handshake 0
	if (d[0] == 0x81 && d[1] == 0x01 && d[2] == 0x00 && d[3] == 0x03) {
		uint8_t reply[64] = {0x80, 0x02};
		hid_write(hid->name, reply, 64);

	// Switch Handshake 1
	} else if (d[0] == 0x81 && d[1] == 0x02 && d[2] == 0x00 && d[3] == 0x00) {
		uint8_t reply[64] = {0x80, 0x04};
		hid_write(hid->name, reply, 64);

	// State
	} else if (d[0] == 0x30) {
		wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = (uint16_t) hid->di.hid.dwVendorId;
		c->pid = (uint16_t) hid->di.hid.dwProductId;
		c->driver = hid->driver;
		c->id = hid->id;

		// Buttons
		c->buttons[0] = d[3] & 0x01;
		c->buttons[1] = d[3] & 0x04;
		c->buttons[2] = d[3] & 0x08;
		c->buttons[3] = d[3] & 0x02;
		c->buttons[4] = d[5] & 0x40;
		c->buttons[5] = d[3] & 0x40;
		c->buttons[6] = d[5] & 0x80;
		c->buttons[7] = d[3] & 0x80;
		c->buttons[8] = d[4] & 0x01;
		c->buttons[9] = d[4] & 0x02;
		c->buttons[10] = d[4] & 0x08;
		c->buttons[11] = d[4] & 0x04;

		// Axis
		uint16_t lx = d[6] | ((d[7] & 0x0F) << 8);
		uint16_t ly = (d[7] >> 4) | (d[8] << 4);
		uint16_t rx = d[9] | ((d[10] & 0x0F) << 8);
		uint16_t ry = (d[10] >> 4) | (d[11] << 4);

		c->values[0].data = lx;
		c->values[0].usage = 0x30;
		c->values[0].min = 300;
		c->values[0].max = 3800;

		c->values[1].data = ly;
		c->values[1].usage = 0x31;
		c->values[1].min = 300;
		c->values[1].max = 3800;

		c->values[2].data = rx;
		c->values[2].usage = 0x32;
		c->values[2].min = 300;
		c->values[2].max = 3800;

		c->values[3].data = ry;
		c->values[3].usage = 0x35;
		c->values[3].min = 300;
		c->values[3].max = 3800;

		// DPAD
		bool up = d[5] & 0x02;
		bool down = d[5] & 0x01;
		bool left = d[5] & 0x08;
		bool right = d[5] & 0x04;

		c->values[4].data = up ? 0 : (up && right) ? 1 : right ? 2 : (right && down) ? 3 :
			down ? 4 : (down && left) ? 5 : left ? 6 : (left && up) ? 7 : 8;
		c->values[4].usage = 0x39;
		c->values[4].min = 0;
		c->values[4].max = 7;

		c->numButtons = 12;
		c->numValues = 5;
	}
}
