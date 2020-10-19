// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct ps4 {
    uint8_t leftX;
    uint8_t leftY;
    uint8_t rightX;
    uint8_t rightY;
    uint8_t buttons[3];
    uint8_t triggerLeft;
    uint8_t triggerRight;
    uint8_t _pad0[3];
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    uint8_t _pad1[5];
    uint8_t batteryLevel;
    uint8_t _pad2[4];
    uint8_t trackpadCounter1;
    uint8_t trackpadData1[3];
    uint8_t trackpadCounter2;
    uint8_t trackpadData2[3];
};

static void hid_ps4_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	const uint8_t *d8 = data;
	const struct ps4 *state = NULL;

	// Wired
	if (d8[0] == 0x01) {
		state = (struct ps4 *) (d8 + 1);

	// Bluetooth
	} else if (d8[0] == 0x11) {
		state = (struct ps4 *) (d8 + 3);

	} else {
		return;
	}

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->numValues = 7;
	c->numButtons = 14;
	c->driver = hid->driver;
	c->id = hid->id;

	c->buttons[MTY_CBUTTON_X] = state->buttons[0] & 0x10;
	c->buttons[MTY_CBUTTON_A] = state->buttons[0] & 0x20;
	c->buttons[MTY_CBUTTON_B] = state->buttons[0] & 0x40;
	c->buttons[MTY_CBUTTON_Y] = state->buttons[0] & 0x80;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = state->buttons[1] & 0x01;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = state->buttons[1] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = state->buttons[1] & 0x04;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = state->buttons[1] & 0x08;
	c->buttons[MTY_CBUTTON_BACK] = state->buttons[1] & 0x10;
	c->buttons[MTY_CBUTTON_START] = state->buttons[1] & 0x20;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = state->buttons[1] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = state->buttons[1] & 0x80;
	c->buttons[MTY_CBUTTON_GUIDE] = state->buttons[2] & 0x01;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = state->buttons[2] & 0x02;

	c->values[MTY_CVALUE_THUMB_LX].data = state->leftX;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = 0;
	c->values[MTY_CVALUE_THUMB_LX].max = UINT8_MAX;

	c->values[MTY_CVALUE_THUMB_LY].data = state->leftY;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = 0;
	c->values[MTY_CVALUE_THUMB_LY].max = UINT8_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = state->rightX;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = 0;
	c->values[MTY_CVALUE_THUMB_RX].max = UINT8_MAX;

	c->values[MTY_CVALUE_THUMB_RY].data = state->rightY;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = 0;
	c->values[MTY_CVALUE_THUMB_RY].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = state->triggerLeft;
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = state->triggerRight;
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->values[MTY_CVALUE_DPAD].data = state->buttons[0] & 0x0F;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}
