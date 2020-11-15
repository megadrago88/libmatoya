// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

enum xbox_proto {
	XBOX_PROTO_UNKNOWN = 0,
	XBOX_PROTO_V1,
	XBOX_PROTO_V2,
};

struct xbox_state {
	enum xbox_proto proto;
	bool guide;
};


// Rumble

static void hid_xbox_rumble(struct hdevice *device, uint16_t low, uint16_t high)
{
	uint8_t rumble[9] = {0x03, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00};
	rumble[4] = low >> 8;
	rumble[5] = high >> 8;

	hid_device_write(device, rumble, 9);
}


// State Reports

static void hid_xbox_init(struct hdevice *device)
{
	hid_xbox_rumble(device, 0, 0);
}

static void hid_xbox_state(struct hdevice *device, const void *data, size_t dsize, MTY_Msg *wmsg)
{
	struct xbox_state *ctx = hid_device_get_state(device);
	const uint8_t *d8 = data;

	if (d8[0] == 0x01) {
		wmsg->type = MTY_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = hid_device_get_vid(device);
		c->pid = hid_device_get_pid(device);
		c->numValues = 7;
		c->numButtons = 14;
		c->driver = MTY_HID_DRIVER_XBOX;
		c->id = hid_device_get_id(device);

		if (ctx->proto == XBOX_PROTO_UNKNOWN && d8[15] & 0x10)
			ctx->proto = XBOX_PROTO_V2;

		if (ctx->proto == XBOX_PROTO_V2)
			ctx->guide = d8[15] & 0x10;

		c->buttons[MTY_CBUTTON_X] = d8[14] & 0x08;
		c->buttons[MTY_CBUTTON_A] = d8[14] & 0x01;
		c->buttons[MTY_CBUTTON_B] = d8[14] & 0x02;
		c->buttons[MTY_CBUTTON_Y] = d8[14] & 0x10;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d8[14] & 0x40;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d8[14] & 0x80;
		c->buttons[MTY_CBUTTON_BACK] = d8[16] & 0x01;
		c->buttons[MTY_CBUTTON_START] = d8[15] & 0x08;
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = d8[15] & 0x20;
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d8[15] & 0x40;
		c->buttons[MTY_CBUTTON_GUIDE] = ctx->guide;

		c->values[MTY_CVALUE_THUMB_LX].data = *((int16_t *) (d8 + 1)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
		c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

		int16_t ly = *((int16_t *) (d8 + 3)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_LY].data = (int16_t) ((int32_t) (ly + 1) * -1);
		c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
		c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

		c->values[MTY_CVALUE_THUMB_RX].data = *((int16_t *) (d8 + 5)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
		c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

		int16_t ry = *((int16_t *) (d8 + 7)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_RY].data = (int16_t) ((int32_t) (ry + 1) * -1);
		c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
		c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

		c->values[MTY_CVALUE_TRIGGER_L].data = *((uint16_t *) (d8 + 9)) >> 2;;
		c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		c->values[MTY_CVALUE_TRIGGER_L].min = 0;
		c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

		c->values[MTY_CVALUE_TRIGGER_R].data = *((uint16_t *) (d8 + 11)) >> 2;;
		c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		c->values[MTY_CVALUE_TRIGGER_R].min = 0;
		c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_L].data > 0;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_R].data > 0;

		c->values[MTY_CVALUE_DPAD].data = d8[13] > 0 ? d8[13] - 1 : 8;
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

	} else if (d8[0] == 0x02) {
		ctx->proto = XBOX_PROTO_V2;
		ctx->guide = d8[1] & 0x01;
	}
}
