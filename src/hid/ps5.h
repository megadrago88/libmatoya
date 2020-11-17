// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct ps5_state {
	bool dummy;
};

static void hid_ps5_init(struct hdevice *device)
{
	/*
	outputReport[0]  = 0x02; // report type
	outputReport[1]  = 0xff; // flags determining what changes this packet will perform
		// 0x01 set the main motors (also requires flag 0x02)
		// 0x02 set the main motors (also requires flag 0x01)
		// 0x04 set the right trigger motor
		// 0x08 set the left trigger motor
		// 0x10 modification of audio volume
		// 0x20 toggling of internal speaker while headset is connected
		// 0x40 modification of microphone volume
		// 0x80 toggling of internal mic or external speaker while headset is connected
	outputReport[2]  = 0xff; // further flags determining what changes this packet will perform
		// 0x01 toggling microphone LED
		// 0x02 toggling audio/mic mute
		// 0x04 toggling LED strips on the sides of the touchpad
		// 0x08 will actively turn all LEDs off? Convenience flag? (if so, third parties might not support it properly)
		// 0x10 toggling white player indicator LEDs below touchpad
		// 0x20 ???
		// 0x40 ??? used by PS Remote Play on startup and exit
		// 0x80 ???

	// main motors
	outputReport[3]  = 0xff; // left low freq motor 0-255
	outputReport[4]  = 0xff; // right high freq motor 0-255

	// audio settings requiring volume control flags
	outputReport[5]  = 0xff; // audio volume of connected headphones (maxes out at about 0x7f)
	outputReport[6]  = 0xff; // volume of internal speaker (0-255) (ties in with index 38?!?)
	outputReport[7]  = 0xff; // internal microphone volume (not at all linear; 0-255, appears to max out at about 64; 0 is not fully muted, use audio mute flag instead!)
	outputReport[8]  = 0x0c; // internal device enablement override flags (0xc default by ps remote play)
		// 0x01 = enable internal microphone (in addition to a connected headset)
		// 0x04 = ??? set by default via PS Remote Play
		// 0x08 = ??? set by default via PS Remote Play
		// 0x10 = disable attached headphones (only if 0x20 to enable internal speakers is provided as well)
		// 0x20 = enable audio on internal speaker (in addition to a connected headset)

	// audio related LEDs requiring according LED toggle flags
	outputReport[9]  = 0x01; // microphone LED (1 = on, 2 = pulsating / neither does affect the mic)

	// audio settings requiring mute toggling flags
	outputReport[10] = 0x00; // 0x10 microphone mute, 0x40 audio mute

	// trigger motors
	outputReport[11] = 0x01; // right trigger motor mode (0 = no resistance, 1 = continuous resistance, 2 = section resistance, 252 = likely a calibration program* / PS Remote Play defaults this to 5; bit 4 only disables the motor?)
	outputReport[12] = 0x2f; // right trigger start of resistance section (0-255; 0 = released state; 0xb0 roughly matches trigger value 0xff)
	outputReport[13] = 0xff; // right trigger
		// (mode1) amount of force exerted; 0-255
		// (mode2) end of resistance section (>= begin of resistance section is enforced); 0xff makes it behave like mode1
	outputReport[14] = 0xff; // right trigger force exerted in range (mode2), 0-255

	outputReport[22] = 0x02; // left trigger motor mode (0 = no resistance, 1 = continuous resistance, 2 = section resistance, 252 = likely a calibration program* / PS Remote Play defaults this to 5; bit 4 only disables the motor?)
	outputReport[23] = 0x2f; // left trigger start of resistance section 0-255 (0 = released state; 0xb0 roughly matches trigger value 0xff)
	outputReport[24] = 0x4f; // left trigger
		// (mode1) amount of force exerted; 0-255
		// (mode2) end of resistance section (>= begin of resistance section is enforced); 0xff makes it behave like mode1
	outputReport[25] = 0xff; // left trigger: (mode2) amount of force exerted within range; 0-255

	outputReport[38] = 0x07; // volume of internal speaker (0-7; ties in with index 6)

	// Uninterruptable Pulse LED setting (requires LED setting flag)
	outputReport[39] = 2; // 2 = blue LED pulse (together with index 42)
	outputReport[42] = 2; // pulse option
			1 = slowly (2s) fade to blue (scheduled to when the regular LED settings are active)
			2 = slowly (2s) fade out (scheduled after fade-in completion) with eventual switch back to configured LED color; only a fade-out can cancel the pulse (neither index 2, 0x08, nor turning this off will cancel it!)

	// Regular LED settings (requires LED setting flag)
	outputReport[44] = 0x04; // 5 white player indicator LEDs below the touchpad (bitmask 00-1f from left to right with 0x04 being the center LED)
	outputReport[45] = 0x1f; // Red value of light bars left and right from touchpad
	outputReport[46] = 0xff; // Green value of light bars left and right from touchpad
	outputReport[47] = 0x1f; // Blue value of light bars left and right from touchpad
	*/
}

static void hid_ps5_state(struct hdevice *device, const void *data, size_t dsize, MTY_Msg *wmsg)
{
	const uint8_t *d = data;

	wmsg->type = MTY_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = hid_device_get_vid(device);
	c->pid = hid_device_get_pid(device);
	c->numValues = 7;
	c->numButtons = 15;
	c->driver = MTY_HID_DRIVER_PS5;
	c->id = hid_device_get_id(device);

	c->buttons[MTY_CBUTTON_X] = d[5] & 0x10;
	c->buttons[MTY_CBUTTON_A] = d[5] & 0x20;
	c->buttons[MTY_CBUTTON_B] = d[5] & 0x40;
	c->buttons[MTY_CBUTTON_Y] = d[5] & 0x80;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[6] & 0x01;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[6] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[6] & 0x04;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[6] & 0x08;
	c->buttons[MTY_CBUTTON_BACK] = d[6] & 0x10;
	c->buttons[MTY_CBUTTON_START] = d[6] & 0x20;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[6] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[6] & 0x80;
	c->buttons[MTY_CBUTTON_GUIDE] = d[7] & 0x01;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = d[7] & 0x02;

	c->values[MTY_CVALUE_THUMB_LX].data = d[1];
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = 0;
	c->values[MTY_CVALUE_THUMB_LX].max = UINT8_MAX;
	hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LX], false);

	c->values[MTY_CVALUE_THUMB_LY].data = d[2];
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = 0;
	c->values[MTY_CVALUE_THUMB_LY].max = UINT8_MAX;
	hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LY], true);

	c->values[MTY_CVALUE_THUMB_RX].data = d[3];
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = 0;
	c->values[MTY_CVALUE_THUMB_RX].max = UINT8_MAX;
	hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RX], false);

	c->values[MTY_CVALUE_THUMB_RY].data = d[4];
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = 0;
	c->values[MTY_CVALUE_THUMB_RY].max = UINT8_MAX;
	hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RY], true);

	c->values[MTY_CVALUE_TRIGGER_L].data = d[8];
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = d[9];
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->values[MTY_CVALUE_DPAD].data = d[5] & 0x0F;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}
