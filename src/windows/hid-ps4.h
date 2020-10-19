// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct ps4 {
    uint8_t leftJoystickX;
    uint8_t leftJoystickY;
    uint8_t rightJoystickX;
    uint8_t rightJoystickY;
    uint8_t buttonsHatAndCounter[3];
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
	if (d8[0] == 1) {
		state = (struct ps4 *) (d8 + 1);

	// Bluetooth
	} else if (d8[0] == 17) {
		state = (struct ps4 *) (d8 + 3);

	} else {
		return;
	}

	printf("%u %u %u %u %u %u\n",
		state->trackpadData1[0], state->trackpadData1[1], state->trackpadData1[2],
		state->trackpadData2[0], state->trackpadData2[1], state->trackpadData2[2]);

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->driver = hid->driver;
	c->id = hid->id;
}
