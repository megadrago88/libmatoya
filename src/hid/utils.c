// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>


// Helpers

void mty_hid_u_to_s16(MTY_Value *v, bool invert)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT16_MAX);
	v->data = (int16_t) (invert ? -(d - INT16_MAX) : (d - INT16_MAX - 1));
	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

void mty_hid_s_to_s16(MTY_Value *v)
{
	if (v->data < 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->min * (float) INT16_MIN);

	if (v->data > 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->max * (float) INT16_MAX);

	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

void mty_hid_u_to_u8(MTY_Value *v)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT8_MAX);
	v->data = (int16_t) d;
	v->min = 0;
	v->max = UINT8_MAX;
}


// Default mapping

static bool hid_swap_value(MTY_Value *values, MTY_CValue a, MTY_CValue b)
{
	if (a != b && values[a].usage != values[b].usage) {
		MTY_Value tmp = values[b];
		values[b] = values[a];
		values[a] = tmp;
		return true;
	}

	return false;
}

static void hid_move_value(MTY_ControllerEvent *c, MTY_Value *v, uint16_t usage,
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

void mty_hid_map_values(MTY_ControllerEvent *c)
{
	// Make sure there is enough room for the standard CVALUEs
	if (c->numValues < MTY_CVALUE_DPAD + 1)
		c->numValues = MTY_CVALUE_DPAD + 1;

	// Swap positions
	for (uint8_t x = 0; x < c->numValues; x++) {
		retry:

		switch (c->values[x].usage) {
			case 0x30: // X -> Left Stick X
				if (hid_swap_value(c->values, x, MTY_CVALUE_THUMB_LX))
					goto retry;
				break;
			case 0x31: // Y -> Left Stick Y
				if (hid_swap_value(c->values, x, MTY_CVALUE_THUMB_LY))
					goto retry;
				break;
			case 0x32: // Z -> Right Stick X
				if (hid_swap_value(c->values, x, MTY_CVALUE_THUMB_RX))
					goto retry;
				break;
			case 0x35: // Rz -> Right Stick Y
				if (hid_swap_value(c->values, x, MTY_CVALUE_THUMB_RY))
					goto retry;
				break;
			case 0x33: // Rx -> Left Trigger
				if (hid_swap_value(c->values, x, MTY_CVALUE_TRIGGER_L))
					goto retry;
				break;
			case 0x34: // Ry -> Right Trigger
				if (hid_swap_value(c->values, x, MTY_CVALUE_TRIGGER_R))
					goto retry;
				break;
			case 0x39: // Hat -> DPAD
				if (hid_swap_value(c->values, x, MTY_CVALUE_DPAD))
					goto retry;
				break;
		}
	}

	// Move values that are not in the right positions to the end
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (x) {
			case MTY_CVALUE_THUMB_LX:  hid_move_value(c, v, 0x30, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_LY:  hid_move_value(c, v, 0x31, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RX:  hid_move_value(c, v, 0x32, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RY:  hid_move_value(c, v, 0x35, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_TRIGGER_L: hid_move_value(c, v, 0x33, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_TRIGGER_R: hid_move_value(c, v, 0x34, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_DPAD:      hid_move_value(c, v, 0x39, 8, 0, 7); break;
		}
	}

	// Convert to int16_t
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (v->usage) {
			case 0x30: // X -> Left Stick X
			case 0x32: // Z -> Right Stick X
				mty_hid_u_to_s16(v, false);
				break;
			case 0x31: // Y -> Left Stick Y
			case 0x35: // Rz -> Right Stick Y
				mty_hid_u_to_s16(v, true);
				break;
			case 0x33: // Rx -> Left Trigger
			case 0x34: // Ry -> Right Trigger
			case 0x36: // Slider
			case 0x37: // Dial
			case 0x38: // Wheel
				mty_hid_u_to_u8(v);
				break;
		}
	}
}


// Deduper

static void hid_clean_value(int16_t *value)
{
	// Dead zone
	if (abs(*value) < 2000)
		*value = 0;

	// Reduce precision
	if (*value != INT16_MIN && *value != INT16_MAX)
		*value &= 0xFFFE;
}

bool mty_hid_dedupe(MTY_Hash *h, MTY_ControllerEvent *c)
{
	MTY_ControllerEvent *prev = MTY_HashGetInt(h, c->id);
	if (!prev) {
		prev = MTY_Alloc(1, sizeof(MTY_ControllerEvent));
		MTY_HashSetInt(h, c->id, prev);
	}

	// Axis dead zone, precision reduction -- helps with deduplication
	hid_clean_value(&c->values[MTY_CVALUE_THUMB_LX].data);
	hid_clean_value(&c->values[MTY_CVALUE_THUMB_LY].data);
	hid_clean_value(&c->values[MTY_CVALUE_THUMB_RX].data);
	hid_clean_value(&c->values[MTY_CVALUE_THUMB_RY].data);

	// Deduplication
	bool button_diff = memcmp(c->buttons, prev->buttons, c->numButtons * sizeof(bool));
	bool values_diff = memcmp(c->values, prev->values, c->numValues * sizeof(MTY_Value));

	*prev = *c;

	return button_diff || values_diff;
}
