// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define PS5_OUTPUT_SIZE 48

struct ps5_state {
	bool init;
	bool bluetooth;
};

static void hid_ps5_rumble(struct hdevice *device, uint16_t low, uint16_t high)
{
	uint8_t buf[PS5_OUTPUT_SIZE] = {
		[0] = 0x02,        // Report Type
		[1] = 0x01 | 0x02, // Flags0: Both motors
		[2] = 0x04 | 0x10, // Flags1: LEDs, player indicator
		[3] = high >> 8,   // Right high freq motor
		[4] = low >> 8,    // Left low freq motor
		[44] = 0x00,       // Player indicator
		[45] = 0xFF,       // LED red
		[46] = 0x00,       // LED green
		[47] = 0xFF,       // LED blue
	};

	hid_device_write(device, buf, PS5_OUTPUT_SIZE);
}

static void hid_ps5_init(struct hdevice *device)
{
	struct ps5_state *ctx = hid_device_get_state(device);
	ctx->bluetooth = false;

	hid_ps5_rumble(device, 0, 0);
}

static void hid_ps5_state(struct hdevice *device, const void *data, size_t dsize, MTY_Msg *wmsg)
{
	struct ps5_state *ctx = hid_device_get_state(device);

	// First write must come after reports start coming in
	if (!ctx->init) {
		hid_ps5_init(device);
		ctx->init = true;

	} else {
		const uint8_t *b = data;
		const uint8_t *a = data;
		const uint8_t *t = data;

		if (!ctx->bluetooth) {
			b += 3;
			t -= 3;
		}

		wmsg->type = MTY_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = hid_device_get_vid(device);
		c->pid = hid_device_get_pid(device);
		c->numValues = 7;
		c->numButtons = 15;
		c->driver = MTY_HID_DRIVER_PS5;
		c->id = hid_device_get_id(device);

		c->buttons[MTY_CBUTTON_X] = b[5] & 0x10;
		c->buttons[MTY_CBUTTON_A] = b[5] & 0x20;
		c->buttons[MTY_CBUTTON_B] = b[5] & 0x40;
		c->buttons[MTY_CBUTTON_Y] = b[5] & 0x80;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = b[6] & 0x01;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = b[6] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = b[6] & 0x04;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = b[6] & 0x08;
		c->buttons[MTY_CBUTTON_BACK] = b[6] & 0x10;
		c->buttons[MTY_CBUTTON_START] = b[6] & 0x20;
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = b[6] & 0x40;
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = b[6] & 0x80;
		c->buttons[MTY_CBUTTON_GUIDE] = b[7] & 0x01;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = b[7] & 0x02;

		c->values[MTY_CVALUE_DPAD].data = b[5] & 0x0F;
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

		c->values[MTY_CVALUE_THUMB_LX].data = a[1];
		c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
		c->values[MTY_CVALUE_THUMB_LX].min = 0;
		c->values[MTY_CVALUE_THUMB_LX].max = UINT8_MAX;
		hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LX], false);

		c->values[MTY_CVALUE_THUMB_LY].data = a[2];
		c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
		c->values[MTY_CVALUE_THUMB_LY].min = 0;
		c->values[MTY_CVALUE_THUMB_LY].max = UINT8_MAX;
		hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LY], true);

		c->values[MTY_CVALUE_THUMB_RX].data = a[3];
		c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
		c->values[MTY_CVALUE_THUMB_RX].min = 0;
		c->values[MTY_CVALUE_THUMB_RX].max = UINT8_MAX;
		hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RX], false);

		c->values[MTY_CVALUE_THUMB_RY].data = a[4];
		c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
		c->values[MTY_CVALUE_THUMB_RY].min = 0;
		c->values[MTY_CVALUE_THUMB_RY].max = UINT8_MAX;
		hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RY], true);

		c->values[MTY_CVALUE_TRIGGER_L].data = t[8];
		c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		c->values[MTY_CVALUE_TRIGGER_L].min = 0;
		c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

		c->values[MTY_CVALUE_TRIGGER_R].data = t[9];
		c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		c->values[MTY_CVALUE_TRIGGER_R].min = 0;
		c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;
	}
}
