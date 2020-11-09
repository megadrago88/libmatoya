// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

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
	for (uint32_t x = 0; x < hid->caps.NumberInputValueCaps && c->numValues < MTY_CVALUE_MAX; x++) {
		const HIDP_VALUE_CAPS *vcap = &hid->vcaps[x];

		ULONG value = 0;
		NTSTATUS e = HidP_GetUsageValue(HidP_Input, vcap->UsagePage, 0, vcap->Range.UsageMin, &value, hid->ppd, data, dsize);
		if (e != HIDP_STATUS_SUCCESS && e != HIDP_STATUS_INCOMPATIBLE_REPORT_ID) {
			MTY_Log("'HidP_GetUsageValue' failed with NTSTATUS 0x%X", e);
			return;
		}

		if (e == HIDP_STATUS_SUCCESS) {
			MTY_Value *v = &c->values[c->numValues++];

			v->usage = vcap->Range.UsageMin;
			v->data = (int16_t) value;
			v->min = (int16_t) vcap->LogicalMin;
			v->max = (int16_t) vcap->LogicalMax;
		}
	}

	wmsg->type = MTY_MSG_CONTROLLER;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->driver = hid->driver;
	c->id = hid->id;

	// Attempt to put values in predictable locations, convert range to int16_t
	hid_default_map_values(c);
}
