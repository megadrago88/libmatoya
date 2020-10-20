// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define HID_NX_OUTPUT_MAX 64
#define HID_NX_USB_MAX    64
#define HID_NX_BT_MAX     49

#pragma pack(1)
struct nx_spi_op {
	uint32_t addr;
	uint8_t len;
};

struct nx_spi_reply {
	struct nx_spi_op op;
	uint8_t data[30];
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
	uint8_t subcommandData[42];
};
#pragma pack()

struct nx_min_max {
	int16_t min;
	int16_t c;
	int16_t max;
};

struct nx_state {
	bool wready;
	bool hs0;
	bool hs1;
	bool hs2;
	bool vibration;
	bool calibration;
	bool calibrated;
	bool home_led;
	bool slot_led;
	bool bluetooth;
	bool usb;
	bool refresh_rumble;
	uint8_t cmd;
	struct nx_min_max lx, slx;
	struct nx_min_max ly, sly;
	struct nx_min_max rx, srx;
	struct nx_min_max ry, sry;
	struct nx_rumble rumble[2];
	int64_t write_ts;
};


// IO Helpers

static uint8_t hid_nx_incr_command(struct nx_state *ctx)
{
	uint8_t v = ctx->cmd;
	ctx->cmd = (ctx->cmd + 1) & 0x0F;

	return v;
}

static void hid_nx_write_hs(struct hid *hid, uint8_t cmd)
{
	struct nx_state *ctx = hid->driver_state;

	uint8_t buf[HID_NX_OUTPUT_MAX] = {0x80};
	buf[1] = cmd;

	hid_write(hid->name, buf, HID_NX_USB_MAX);

	ctx->write_ts = MTY_Timestamp();
	ctx->wready = false;
}

static void hid_nx_write_command(struct hid *hid, uint8_t command, const void *data, size_t size)
{
	struct nx_state *ctx = hid->driver_state;

	uint8_t buf[HID_NX_OUTPUT_MAX] = {0};
	struct nx_subcommand *out = (struct nx_subcommand *) buf;

	out->common.packetType = 0x01;
	out->common.packetNumber = hid_nx_incr_command(ctx);
	out->common.rumble[0] = ctx->rumble[0];
	out->common.rumble[1] = ctx->rumble[1];
	out->subcommandID = command;
	memcpy(out->subcommandData, data, size);

	hid_write(hid->name, buf, ctx->bluetooth ? HID_NX_BT_MAX : HID_NX_USB_MAX);

	ctx->write_ts = MTY_Timestamp();
	ctx->wready = false;
}


// Rumble

static void hid_nx_rumble_off(struct nx_rumble *rmb)
{
	rmb->data[0] = 0x00;
	rmb->data[1] = 0x01;
	rmb->data[2] = 0x40;
	rmb->data[3] = 0x40;
}

static void hid_nx_rumble_on(struct nx_rumble *rmb)
{
	rmb->data[0] = 0x74;
	rmb->data[1] = 0xBE;
	rmb->data[2] = 0xBD;
	rmb->data[3] = 0x6F;
}

static void hid_nx_rumble(struct hid *hid, bool low, bool high)
{
	struct nx_state *ctx = hid->driver_state;

	hid_nx_rumble_off(&ctx->rumble[0]);
	hid_nx_rumble_off(&ctx->rumble[1]);

	if (low)
		hid_nx_rumble_on(&ctx->rumble[0]);

	if (high)
		hid_nx_rumble_on(&ctx->rumble[1]);

	ctx->refresh_rumble = low || high;
	ctx->vibration = false;
}


// State Reports

static void hid_nx_full_state(struct hid *hid, const uint8_t *d, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;

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

	ctx->lx.min = lx < ctx->lx.min ? lx : ctx->lx.min;
	ctx->ly.min = ly < ctx->ly.min ? ly : ctx->ly.min;
	ctx->rx.min = rx < ctx->rx.min ? rx : ctx->rx.min;
	ctx->ry.min = ry < ctx->ry.min ? ry : ctx->ry.min;
	ctx->lx.max = lx > ctx->lx.max ? lx : ctx->lx.max;
	ctx->ly.max = ly > ctx->ly.max ? ly : ctx->ly.max;
	ctx->rx.max = rx > ctx->rx.max ? rx : ctx->rx.max;
	ctx->ry.max = ry > ctx->ry.max ? ry : ctx->ry.max;

	c->values[MTY_CVALUE_THUMB_LX].data = lx;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = ctx->lx.min;
	c->values[MTY_CVALUE_THUMB_LX].max = ctx->lx.max;

	c->values[MTY_CVALUE_THUMB_LY].data = ly;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = ctx->ly.min;
	c->values[MTY_CVALUE_THUMB_LY].max = ctx->ly.max;

	c->values[MTY_CVALUE_THUMB_RX].data = rx;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = ctx->rx.min;
	c->values[MTY_CVALUE_THUMB_RX].max = ctx->rx.max;

	c->values[MTY_CVALUE_THUMB_RY].data = ry;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = ctx->ry.min;
	c->values[MTY_CVALUE_THUMB_RY].max = ctx->ry.max;

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
}

static void hid_nx_simple_state(struct hid *hid, const uint8_t *d, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->numValues = 7;
	c->numButtons = 14;
	c->driver = hid->driver;
	c->id = hid->id;

	c->buttons[MTY_CBUTTON_X] = d[1] & 0x04;
	c->buttons[MTY_CBUTTON_A] = d[1] & 0x01;
	c->buttons[MTY_CBUTTON_B] = d[1] & 0x02;
	c->buttons[MTY_CBUTTON_Y] = d[1] & 0x08;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[1] & 0x10;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[1] & 0x20;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[1] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[1] & 0x80;
	c->buttons[MTY_CBUTTON_BACK] = d[2] & 0x01;
	c->buttons[MTY_CBUTTON_START] = d[2] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[2] & 0x04;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[2] & 0x08;
	c->buttons[MTY_CBUTTON_GUIDE] = d[2] & 0x20;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = d[2] & 0x10;

	int16_t lx = (d[4] | (d[5] << 8)) - ctx->slx.c;
	int16_t ly = (d[6] | (d[7] << 8)) - ctx->sly.c;
	int16_t rx = (d[8] | (d[9] << 8)) - ctx->srx.c;
	int16_t ry = (d[10] | (d[11] << 8)) - ctx->sry.c;

	ctx->slx.min = lx < ctx->slx.min ? lx : ctx->slx.min;
	ctx->sly.min = ly < ctx->sly.min ? ly : ctx->sly.min;
	ctx->srx.min = rx < ctx->srx.min ? rx : ctx->srx.min;
	ctx->sry.min = ry < ctx->sry.min ? ry : ctx->sry.min;
	ctx->slx.max = lx > ctx->slx.max ? lx : ctx->slx.max;
	ctx->sly.max = ly > ctx->sly.max ? ly : ctx->sly.max;
	ctx->srx.max = rx > ctx->srx.max ? rx : ctx->srx.max;
	ctx->sry.max = ry > ctx->sry.max ? ry : ctx->sry.max;

	c->values[MTY_CVALUE_THUMB_LX].data = lx;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = ctx->slx.min;
	c->values[MTY_CVALUE_THUMB_LX].max = ctx->slx.max;

	c->values[MTY_CVALUE_THUMB_LY].data = ly;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = ctx->sly.min;
	c->values[MTY_CVALUE_THUMB_LY].max = ctx->sly.max;

	c->values[MTY_CVALUE_THUMB_RX].data = rx;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = ctx->srx.min;
	c->values[MTY_CVALUE_THUMB_RX].max = ctx->srx.max;

	c->values[MTY_CVALUE_THUMB_RY].data = ry;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = ctx->sry.min;
	c->values[MTY_CVALUE_THUMB_RY].max = ctx->sry.max;

	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->values[MTY_CVALUE_DPAD].data = d[3];
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}


// Full Stick Calibration Data

static void hid_nx_parse_spi(struct hid *hid, const uint8_t *d)
{
	struct nx_state *ctx = hid->driver_state;
	const struct nx_spi_reply *spi = (const struct nx_spi_reply *) d;

	// Left Stick
	uint16_t lx_max = ((spi->data[1] << 8) & 0xF00) | spi->data[0];
	uint16_t ly_max = (spi->data[2] << 4) | (spi->data[1] >> 4);
	uint16_t lx_c = ((spi->data[4] << 8) & 0xF00) | spi->data[3];
	uint16_t ly_c = (spi->data[5] << 4) | (spi->data[4] >> 4);
	uint16_t lx_min = ((spi->data[7] << 8) & 0xF00) | spi->data[6];
	uint16_t ly_min = (spi->data[8] << 4) | (spi->data[7] >> 4);
	ctx->lx.c = lx_c;
	ctx->lx.min = (int16_t) lrint((float) (lx_c - lx_min - lx_c) * 0.7f);
	ctx->lx.max = (int16_t) lrint((float) (lx_c + lx_max - lx_c) * 0.7f);
	ctx->ly.c = ly_c;
	ctx->ly.min = (int16_t) lrint((float) (ly_c - ly_min - ly_c) * 0.7f);
	ctx->ly.max = (int16_t) lrint((float) (ly_c + ly_max - ly_c) * 0.7f);

	// Right Stick
	uint16_t rx_c = ((spi->data[10] << 8) & 0xF00) | spi->data[9];
	uint16_t ry_c = (spi->data[11] << 4) | (spi->data[10] >> 4);
	uint16_t rx_min = ((spi->data[13] << 8) & 0xF00) | spi->data[12];
	uint16_t ry_min = (spi->data[14] << 4) | (spi->data[13] >> 4);
	uint16_t rx_max = ((spi->data[16] << 8) & 0xF00) | spi->data[15];
	uint16_t ry_max = (spi->data[17] << 4) | (spi->data[16] >> 4);
	ctx->rx.c = rx_c;
	ctx->rx.min = (int16_t) lrint((float) (rx_c - rx_min - rx_c) * 0.7f);
	ctx->rx.max = (int16_t) lrint((float) (rx_c + rx_max - rx_c) * 0.7f);
	ctx->ry.c = ry_c;
	ctx->ry.min = (int16_t) lrint((float) (ry_c - ry_min - ry_c) * 0.7f);
	ctx->ry.max = (int16_t) lrint((float) (ry_c + ry_max - ry_c) * 0.7f);
}


// IO State Machine

static void hid_nx_init(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;

	// Neutral rumble
	hid_nx_rumble_off(&ctx->rumble[0]);
	hid_nx_rumble_off(&ctx->rumble[1]);

	// Simple calibration
	ctx->slx.c = ctx->sly.c = ctx->srx.c = ctx->sry.c = 0x8000;
	ctx->slx.min = ctx->sly.min = ctx->srx.min = ctx->sry.min = INT16_MIN / 2;
	ctx->slx.max = ctx->sly.max = ctx->srx.max = ctx->sry.max = INT16_MAX / 2;

	// USB init packet (begins handshake)
	hid_nx_write_hs(hid, 0x02);
}

static void hid_nx_state_machine(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;

	bool timeout = MTY_TimeDiff(ctx->write_ts, MTY_Timestamp()) > 500.0f;

	// USB Handshake is used to decide whether we're dealing with bluetooth or not
	if (!ctx->usb && !ctx->bluetooth && timeout)
		ctx->bluetooth = true;

	if (timeout)
		ctx->wready = true;

	// Once we know if we're USB or Bluetooth, start the state machine
	if ((ctx->usb || ctx->bluetooth)) {
		// USB Only
		if (ctx->usb) {
			// Set speed to 3mbit, may timeout on certain devices
			if (ctx->wready && !ctx->hs0) {
				hid_nx_write_hs(hid, 0x03);
				ctx->hs0 = true;
			}

			// Send an additional handshake packet after baud rate
			if (ctx->wready && !ctx->hs1) {
				hid_nx_write_hs(hid, 0x02);
				ctx->hs1 = true;
			}

			// Force USB
			if (ctx->wready && !ctx->hs2) {
				hid_nx_write_hs(hid, 0x04);
				ctx->wready = true; // There is no response to this
				ctx->hs2 = true;
			}
		}

		// Calibration (SPI Flash Read)
		if (ctx->wready && !ctx->calibration) {
			struct nx_spi_op op = {0x603D, 18};
			hid_nx_write_command(hid, 0x10, &op, sizeof(struct nx_spi_op));
			ctx->calibration = true;
		}

		// Enable vibration
		if (ctx->wready && !ctx->vibration) {
			uint8_t enabled = 1;
			hid_nx_write_command(hid, 0x48, &enabled, 1);
			ctx->vibration = true;
		}

		// Home LED
		if (ctx->wready && !ctx->home_led) {
			uint8_t buf[4] = {0x01, 0xF0, 0xF0, 0x00};
			hid_nx_write_command(hid, 0x38, buf, 4);
			ctx->home_led = true;
		}

		// Player Slot LED
		if (ctx->wready && !ctx->slot_led) {
			uint8_t data = 0x01; // Always "Player 1" TODO? Generate a real player ID
			hid_nx_write_command(hid, 0x30, &data, 1);
			ctx->slot_led = true;
		}
	}
}

static void hid_nx_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;
	const uint8_t *d = data;

	// State (Full)
	if (d[0] == 0x30 && ctx->calibrated) {
		hid_nx_full_state(hid, d, wmsg);

	// State (Simple)
	} else if (d[0] == 0x3F && ctx->calibrated) {
		hid_nx_simple_state(hid, d, wmsg);

	// USB Handshake Response
	} else if (d[0] == 0x81 && (d[1] == 0x02 || d[1] == 0x03 || d[2] == 0x04)) {
		ctx->wready = true;
		ctx->usb = true;

	// Subcommand Response
	} else if (d[0] == 0x21) {
		uint8_t subcommand = d[14];
		ctx->wready = true;

		switch (subcommand) {
			// SPI Flash Read
			case 0x10: {
				hid_nx_parse_spi(hid, d + 15);
				ctx->calibrated = true;
				break;
			}
		}
	}

	hid_nx_state_machine(hid);
}
