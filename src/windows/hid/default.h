// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

static void hid_default_state(struct hid *hid, void *data, ULONG dsize, MTY_Msg *wmsg)
{
	MTY_Controller *c = &wmsg->controller;

	// Buttons
	for (uint32_t x = 0; x < hid->caps.NumberInputButtonCaps; x++) {
		const HIDP_BUTTON_CAPS *bcap = &hid->bcaps[x];

		// Should we consider usages other than 0x09 (Button)?
		if (bcap->UsagePage == 0x09) {
			c->numButtons += (uint8_t) (bcap->Range.UsageMax - bcap->Range.UsageMin + 1);

			ULONG n = MTY_CBUTTON_MAX;
			USAGE usages[MTY_CBUTTON_MAX];
			NTSTATUS e = HidP_GetUsages(HidP_Input, bcap->UsagePage, 0, usages, &n, hid->ppd, data, dsize);
			if (e != HIDP_STATUS_SUCCESS && e != HIDP_STATUS_INCOMPATIBLE_REPORT_ID) {
				MTY_Log("'HidP_GetUsages' failed with NTSTATUS 0x%X", e);
				return;
			}

			if (e == HIDP_STATUS_SUCCESS) {
				for (ULONG y = 0; y < n; y++) {
					uint32_t i = usages[y] - bcap->Range.UsageMin;
					if (i < MTY_CBUTTON_MAX)
						c->buttons[i] = true;
				}
			}
		}
	}

	// Values
	for (uint32_t x = 0; x < hid->caps.NumberInputValueCaps && x < MTY_CVALUE_MAX; x++) {
		const HIDP_VALUE_CAPS *vcap = &hid->vcaps[x];

		ULONG value = 0;
		NTSTATUS e = HidP_GetUsageValue(HidP_Input, vcap->UsagePage, 0, vcap->Range.UsageMin, &value, hid->ppd, data, dsize);
		if (e != HIDP_STATUS_SUCCESS && e != HIDP_STATUS_INCOMPATIBLE_REPORT_ID) {
			MTY_Log("'HidP_GetUsageValue' failed with NTSTATUS 0x%X", e);
			return;
		}

		if (e == HIDP_STATUS_SUCCESS) {
			c->values[x].usage = vcap->Range.UsageMin;
			c->values[x].data = (int16_t) value;
			c->values[x].min = (int16_t) vcap->LogicalMin;
			c->values[x].max = (int16_t) vcap->LogicalMax;

			c->numValues++;
		}
	}

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->driver = hid->driver;
	c->id = hid->id;
}


// Default Map

static void hid_fit_to_s16(MTY_Controller *c, uint8_t i, bool invert)
{
	float v = (float) c->values[i].data - (float) c->values[i].min;
	float max = (float) c->values[i].max - (float) c->values[i].min;

	int32_t d = lrint((v / max) * (float) UINT16_MAX);
	c->values[i].data = (int16_t) (invert ? -(d - INT16_MAX) : (d - INT16_MAX - 1));
	c->values[i].min = INT16_MIN;
	c->values[i].max = INT16_MAX;
}

static void hid_fit_to_u8(MTY_Controller *c, uint8_t i)
{
	float v = (float) c->values[i].data - (float) c->values[i].min;
	float max = (float) c->values[i].max - (float) c->values[i].min;

	int32_t d = lrint((v / max) * (float) UINT8_MAX);
	c->values[i].data = (int16_t) d;
	c->values[i].min = 0;
	c->values[i].max = UINT8_MAX;
}

static MTY_Controller hid_default_map(const MTY_Controller *c)
{
	if (c->driver == MTY_HID_DRIVER_XINPUT)
		return *c;

	MTY_Controller m = {0};
	m.driver = c->driver;
	m.id = c->id;
	m.vid = c->vid;
	m.pid = c->pid;
	m.numButtons = 14;
	m.numValues = 6;

	// Values
	bool have_lt = false;
	bool have_rt = false;

	for (uint8_t x = 0; x < c->numValues; x++) {
		switch (c->values[x].usage) {
			case 0x30: // X -> Left Stick X
				m.values[MTY_CVALUE_THUMB_LX] = c->values[x];
				hid_fit_to_s16(&m, MTY_CVALUE_THUMB_LX, false);
				break;
			case 0x31: // Y -> Left Stick Y
				m.values[MTY_CVALUE_THUMB_LY] = c->values[x];
				hid_fit_to_s16(&m, MTY_CVALUE_THUMB_LY, true);
				break;
			case 0x32: // Z -> Right Stick X
				m.values[MTY_CVALUE_THUMB_RX] = c->values[x];
				hid_fit_to_s16(&m, MTY_CVALUE_THUMB_RX, false);
				break;
			case 0x35: // Rz -> Right Stick Y
				m.values[MTY_CVALUE_THUMB_RY] = c->values[x];
				hid_fit_to_s16(&m, MTY_CVALUE_THUMB_RY, true);
				break;
			case 0x33: // Rx -> Left Trigger
				m.values[MTY_CVALUE_TRIGGER_L] = c->values[x];
				hid_fit_to_u8(&m, MTY_CVALUE_TRIGGER_L);
				have_lt = true;
				break;
			case 0x34: // Ry -> Right Trigger
				m.values[MTY_CVALUE_TRIGGER_R] = c->values[x];
				hid_fit_to_u8(&m, MTY_CVALUE_TRIGGER_R);
				have_rt = true;
				break;
			case 0x39: // Hat -> DPAD
				m.values[MTY_CVALUE_DPAD] = c->values[x];
				break;
		}
	}

	// Buttons
	for (uint8_t x = 0; x < c->numButtons; x++)
		m.buttons[x] = c->buttons[x];

	// Buttons -> Values
	if (!have_lt) {
		m.values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		m.values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
		m.values[MTY_CVALUE_TRIGGER_L].min = 0;
		m.values[MTY_CVALUE_TRIGGER_L].max = INT16_MAX;
	}

	if (!have_rt) {
		m.values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		m.values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
		m.values[MTY_CVALUE_TRIGGER_R].min = 0;
		m.values[MTY_CVALUE_TRIGGER_R].max = INT16_MAX;
	}

	return m;
}
