// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define HID_NX_SUBCOMMAND_MAX 42
#define HID_NX_OUTPUT_MAX     64


// Formatted Buffers

#pragma pack(1)
struct nx_spi_op {
	uint32_t addr;
	uint8_t len;
};

struct nx_spi_reply {
	struct nx_spi_op op;
	uint8_t data[35 - sizeof(struct nx_spi_op)];
};

struct nx_rumble {
	uint8_t data[4];
} rumble;

struct nx_output_common {
	uint8_t packetType;
	uint8_t packetNumber;
	struct nx_rumble rumble[2];
} common;

struct nx_subcommand {
	struct nx_output_common common;
    uint8_t subcommandID;
    uint8_t subcommandData[HID_NX_SUBCOMMAND_MAX];
};
#pragma pack()


// NX Protocol

struct nx_min_max {
	int16_t min;
	int16_t c;
	int16_t max;
};

struct nx_state {
	bool reply;
	bool vibration;
	bool calibration;
	bool calibrated;
	bool home_led;
	bool slot_led;
	struct nx_min_max lx;
	struct nx_min_max ly;
	struct nx_min_max rx;
	struct nx_min_max ry;
	struct nx_rumble rumble[2];
};

static uint8_t HID_NX_COMMAND;

static uint8_t hid_nx_incr_command(void)
{
	uint8_t v = HID_NX_COMMAND;
	HID_NX_COMMAND = (HID_NX_COMMAND + 1) & 0x0F;

	return v;
}

static void hid_nx_write_command(struct hid *hid, uint8_t command, const void *data, size_t size)
{
	struct nx_state *ctx = hid->driver_state;

	uint8_t buf[HID_NX_OUTPUT_MAX] = {0};
	struct nx_subcommand *out = (struct nx_subcommand *) buf;

	out->common.packetType = 0x01;
	out->common.packetNumber = hid_nx_incr_command();
	out->common.rumble[0] = ctx->rumble[0];
	out->common.rumble[1] = ctx->rumble[1];
	out->subcommandID = command;
	memcpy(out->subcommandData, data, size);

	hid_write(hid->name, buf, HID_NX_OUTPUT_MAX);
	ctx->reply = false;
}

static void hid_nx_init(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;
	ctx->reply = true;

	// Neutral rumble
	ctx->rumble[0].data[0] = 0x00;
	ctx->rumble[0].data[1] = 0x01;
	ctx->rumble[0].data[2] = 0x40;
	ctx->rumble[0].data[3] = 0x40;
	ctx->rumble[1] = ctx->rumble[0];

	// USB init packet (begins handshake)
	uint8_t reply[HID_NX_OUTPUT_MAX] = {0x80, 0x02};
	hid_write(hid->name, reply, HID_NX_OUTPUT_MAX);
	ctx->reply = false;
}

static void hid_nx_state_machine(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;

	// Calibration (SPI Flash Read)
	if (ctx->reply && !ctx->calibration) {
		struct nx_spi_op op = {0x603D, 18};
		hid_nx_write_command(hid, 0x10, &op, sizeof(struct nx_spi_op));
		ctx->calibration = true;
	}

	// Enable vibration
	if (ctx->reply && !ctx->vibration) {
		uint8_t enabled = 1;
		hid_nx_write_command(hid, 0x48, &enabled, 1);
		ctx->vibration = true;
	}

	// TODO? Set Input Mode

	// Home LED
	if (ctx->reply && !ctx->home_led) {
		uint8_t buf[4] = {0x01, 0xF0, 0xF0, 0x00};
		hid_nx_write_command(hid, 0x38, buf, 4);
		ctx->home_led = true;
	}

	// Player Slot LED
	if (ctx->reply && !ctx->slot_led) {
		uint8_t data = 0x01; // Always "Player 1" TODO? Generate a real player ID
		hid_nx_write_command(hid, 0x30, &data, 1);
		ctx->slot_led = true;
	}
}

static void hid_nx_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;
	const uint8_t *d = data;

	hid_nx_state_machine(hid);

	// Switch USB Handshake
	if (d[0] == 0x81 && d[1] == 0x02 && d[2] == 0x00 && d[3] == 0x00) {
		uint8_t reply[HID_NX_OUTPUT_MAX] = {0x80, 0x04}; // Force USB
		hid_write(hid->name, reply, HID_NX_OUTPUT_MAX);
		ctx->reply = true;

	// State (Full)
	} else if (d[0] == 0x30 && ctx->calibrated) {
		wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = (uint16_t) hid->di.hid.dwVendorId;
		c->pid = (uint16_t) hid->di.hid.dwProductId;
		c->numValues = 7;
		c->numButtons = 14;
		c->driver = hid->driver;
		c->id = hid->id;

		c->buttons[MTY_CBUTTON_X] = d[3] & 0x01;
		c->buttons[MTY_CBUTTON_A] = d[3] & 0x04;
		c->buttons[MTY_CBUTTON_B] = d[3] & 0x08;
		c->buttons[MTY_CBUTTON_Y] = d[3] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[5] & 0x40;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[3] & 0x40;
		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[5] & 0x80;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[3] & 0x80;
		c->buttons[MTY_CBUTTON_BACK] = d[4] & 0x01;
		c->buttons[MTY_CBUTTON_START] = d[4] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[4] & 0x08;
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[4] & 0x04;
		c->buttons[MTY_CBUTTON_GUIDE] = d[4] & 0x10;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = d[4] & 0x20; // The "Capture" button

		int16_t lx = (d[6] | ((d[7] & 0x0F) << 8)) - ctx->lx.c;
		int16_t ly = ((d[7] >> 4) | (d[8] << 4)) - ctx->ly.c;
		int16_t rx = (d[9] | ((d[10] & 0x0F) << 8)) - ctx->rx.c;
		int16_t ry = ((d[10] >> 4) | (d[11] << 4)) - ctx->ry.c;

		c->values[MTY_CVALUE_THUMB_LX].data = lx > ctx->lx.max ? ctx->lx.max : lx < ctx->lx.min ? ctx->lx.min : lx;
		c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
		c->values[MTY_CVALUE_THUMB_LX].min = ctx->lx.min;
		c->values[MTY_CVALUE_THUMB_LX].max = ctx->lx.max;

		c->values[MTY_CVALUE_THUMB_LY].data = -(ly > ctx->ly.max ? ctx->ly.max : ly < ctx->ly.min ? ctx->ly.min : ly);
		c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
		c->values[MTY_CVALUE_THUMB_LY].min = -ctx->ly.max;
		c->values[MTY_CVALUE_THUMB_LY].max = -ctx->ly.min;

		c->values[MTY_CVALUE_THUMB_RX].data = rx > ctx->rx.max ? ctx->rx.max : rx < ctx->rx.min ? ctx->rx.min : rx;
		c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
		c->values[MTY_CVALUE_THUMB_RX].min = ctx->rx.min;
		c->values[MTY_CVALUE_THUMB_RX].max = ctx->rx.max;

		c->values[MTY_CVALUE_THUMB_RY].data = -(ry > ctx->ry.max ? ctx->ry.max : ry < ctx->ry.min ? ctx->ry.min : ry);
		c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
		c->values[MTY_CVALUE_THUMB_RY].min = -ctx->ry.max;
		c->values[MTY_CVALUE_THUMB_RY].max = -ctx->ry.min;

		c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		c->values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
		c->values[MTY_CVALUE_TRIGGER_L].min = 0;
		c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

		c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		c->values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
		c->values[MTY_CVALUE_TRIGGER_R].min = 0;
		c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

		bool up = d[5] & 0x02;
		bool down = d[5] & 0x01;
		bool left = d[5] & 0x08;
		bool right = d[5] & 0x04;

		c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
			(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

	// State (Simple)
	} else if (d[0] == 0x3F) {
		wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

		MTY_Controller *c = &wmsg->controller;
		c->vid = (uint16_t) hid->di.hid.dwVendorId;
		c->pid = (uint16_t) hid->di.hid.dwProductId;
		c->numValues = 7;
		c->numButtons = 14;
		c->driver = hid->driver;
		c->id = hid->id;

		c->buttons[MTY_CBUTTON_X] = d[1] & 0x01;
		c->buttons[MTY_CBUTTON_A] = d[1] & 0x04;
		c->buttons[MTY_CBUTTON_B] = d[1] & 0x08;
		c->buttons[MTY_CBUTTON_Y] = d[1] & 0x02;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[1] & 0x10;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[1] & 0x20;
		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = false;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = false;
		c->buttons[MTY_CBUTTON_BACK] = (d[2] & 0x01) || (d[2] & 0x10);  // Minus || Home
		c->buttons[MTY_CBUTTON_START] = (d[2] & 0x02) || (d[2] & 0x20); // Home || Capture
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = (d[2] & 0x04) || (d[2] & 0x08); // Left Stick || Right Stick
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = false;
		c->buttons[MTY_CBUTTON_GUIDE] = false;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = false;

		c->values[MTY_CVALUE_DPAD].data = d[3];
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

	// Subcommand Response
	} else if (d[0] == 0x21) {
		ctx->reply = true;

		switch (d[14]) {
			case 0x10: { // SPI Flash Read
				const struct nx_spi_reply *spi = (const struct nx_spi_reply *) (d + 15);

				// Left Stick
				uint16_t lx_max = ((spi->data[1] << 8) & 0xF00) | spi->data[0];
				uint16_t ly_max = (spi->data[2] << 4) | (spi->data[1] >> 4);
				uint16_t lx_c = ((spi->data[4] << 8) & 0xF00) | spi->data[3];
				uint16_t ly_c = (spi->data[5] << 4) | (spi->data[4] >> 4);
				uint16_t lx_min = ((spi->data[7] << 8) & 0xF00) | spi->data[6];
				uint16_t ly_min = (spi->data[8] << 4) | (spi->data[7] >> 4);
				ctx->lx.c = lx_c;
				ctx->lx.min = lx_c - lx_min - lx_c;
				ctx->lx.max = lx_c + lx_max - lx_c;
				ctx->ly.c = ly_c;
				ctx->ly.min = ly_c - ly_min - ly_c;
				ctx->ly.max = ly_c + ly_max - ly_c;

				// Right Stick
				uint16_t rx_c = ((spi->data[10] << 8) & 0xF00) | spi->data[9];
				uint16_t ry_c = (spi->data[11] << 4) | (spi->data[10] >> 4);
				uint16_t rx_min = ((spi->data[13] << 8) & 0xF00) | spi->data[12];
				uint16_t ry_min = (spi->data[14] << 4) | (spi->data[13] >> 4);
				uint16_t rx_max = ((spi->data[16] << 8) & 0xF00) | spi->data[15];
				uint16_t ry_max = (spi->data[17] << 4) | (spi->data[16] >> 4);
				ctx->rx.c = rx_c;
				ctx->rx.min = rx_c - rx_min - rx_c;
				ctx->rx.max = rx_c + rx_max - rx_c;
				ctx->ry.c = ry_c;
				ctx->ry.min = ry_c - ry_min - ry_c;
				ctx->ry.max = ry_c + ry_max - ry_c;

				ctx->calibrated = true;
				break;
			}
		}
	}
}
