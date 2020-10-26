// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

static void hid_default_swap_value(MTY_Value *values, MTY_CValue i, MTY_Value *v)
{
	MTY_Value tmp = values[i];
	values[i] = *v;
	*v = tmp;
}

static void hid_default_map_values(MTY_Controller *c)
{
	bool have_lt = false;
	bool have_rt = false;

	// Make sure there is enough room for the standard CVALUEs
	if (c->numValues < MTY_CVALUE_DPAD + 1)
		c->numValues = MTY_CVALUE_DPAD + 1;

	// Swap positions
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		retry:

		switch (v->usage) {
			case 0x30: // X -> Left Stick X
				if (x != MTY_CVALUE_THUMB_LX) {
					hid_default_swap_value(c->values, MTY_CVALUE_THUMB_LX, v);
					goto retry;
				}
				break;
			case 0x31: // Y -> Left Stick Y
				if (x != MTY_CVALUE_THUMB_LY) {
					hid_default_swap_value(c->values, MTY_CVALUE_THUMB_LY, v);
					goto retry;
				}
				break;
			case 0x32: // Z -> Right Stick X
				if (x != MTY_CVALUE_THUMB_RX) {
					hid_default_swap_value(c->values, MTY_CVALUE_THUMB_RX, v);
					goto retry;
				}
				break;
			case 0x35: // Rz -> Right Stick Y
				if (x != MTY_CVALUE_THUMB_RY) {
					hid_default_swap_value(c->values, MTY_CVALUE_THUMB_RY, v);
					goto retry;
				}
				break;
			case 0x33: // Rx -> Left Trigger
				have_lt = true;
				if (x != MTY_CVALUE_TRIGGER_L) {
					hid_default_swap_value(c->values, MTY_CVALUE_TRIGGER_L, v);
					goto retry;
				}
				break;
			case 0x34: // Ry -> Right Trigger
				have_rt = true;
				if (x != MTY_CVALUE_TRIGGER_R) {
					hid_default_swap_value(c->values, MTY_CVALUE_TRIGGER_R, v);
					goto retry;
				}
				break;
			case 0x39: // Hat -> DPAD
				if (x != MTY_CVALUE_DPAD) {
					hid_default_swap_value(c->values, MTY_CVALUE_DPAD, v);
					goto retry;
				}
				break;
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

			// Rx -> Left Trigger
			// Ry -> Right Trigger
			// Catch all
			default:
				hid_u_to_u8(v);
				break;
		}
	}

	// Add left and right trigger values if necessary
	if (!have_lt && c->numValues < MTY_CVALUE_MAX) {
		MTY_Value *v = &c->values[c->numValues++];

		v->usage = 0x33;
		v->data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
		v->min = 0;
		v->max = UINT8_MAX;

		hid_default_swap_value(c->values, MTY_CVALUE_TRIGGER_L, v);

		if (v->usage == 0x00)
			c->numValues--;
	}

	if (!have_rt && c->numValues < MTY_CVALUE_MAX) {
		MTY_Value *v = &c->values[c->numValues++];

		v->usage = 0x34;
		v->data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
		v->min = 0;
		v->max = UINT8_MAX;

		hid_default_swap_value(c->values, MTY_CVALUE_TRIGGER_R, v);

		if (v->usage == 0x00)
			c->numValues--;
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

	// Attempt to put values in predictable locations, convert range to int16_t
	hid_default_map_values(c);
}
