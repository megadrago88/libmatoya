// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <xinput.h>

typedef struct {
	WORD vid;
	WORD pid;
	uint8_t _pad[64];
} XINPUT_BASE_BUS_INFORMATION;

struct xip {
	bool disabled;
	bool was_enabled;
	DWORD packet;
	XINPUT_BASE_BUS_INFORMATION bbi;
};

static HMODULE XINPUT1_4;
static DWORD (WINAPI *_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE *pState) = XInputGetState;
static DWORD (WINAPI *_XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration) = XInputSetState;
static DWORD (WINAPI *_XInputGetBaseBusInformation)(DWORD dwUserIndex, XINPUT_BASE_BUS_INFORMATION *pBusInformation);

static void hid_xinput_init(void)
{
	XINPUT1_4 = LoadLibrary(L"xinput1_4.dll");
	if (XINPUT1_4) {
		_XInputGetState = (void *) GetProcAddress(XINPUT1_4, "XInputGetState");
		_XInputSetState = (void *) GetProcAddress(XINPUT1_4, "XInputSetState");
		_XInputGetBaseBusInformation = (void *) GetProcAddress(XINPUT1_4, (LPCSTR) 104);
	}
}

static void hid_xinput_destroy(void)
{
	if (XINPUT1_4) {
		_XInputGetState = XInputGetState;
		_XInputSetState = XInputSetState;
		_XInputGetBaseBusInformation = NULL;

		FreeLibrary(XINPUT1_4);
	}
}

static void hid_xinput_refresh(struct xip *xip)
{
	for (uint8_t x = 0; x < 4; x++)
		xip[x].disabled = false;
}

static void hid_xinput_rumble(struct xip *xip, uint32_t id, uint16_t low, uint16_t high)
{
	if (id >= 4)
		return;

	if (!xip[id].disabled) {
		XINPUT_VIBRATION vb = {0};
		vb.wLeftMotorSpeed = low;
		vb.wRightMotorSpeed = high;

		_XInputSetState(id, &vb);
	}
}

static void hid_xinput_to_mty(const XINPUT_STATE *xstate, MTY_Msg *wmsg)
{
	WORD b = xstate->Gamepad.wButtons;
	MTY_Controller *c = &wmsg->controller;

	c->buttons[MTY_CBUTTON_A] = b & XINPUT_GAMEPAD_A;
	c->buttons[MTY_CBUTTON_B] = b & XINPUT_GAMEPAD_B;
	c->buttons[MTY_CBUTTON_X] = b & XINPUT_GAMEPAD_X;
	c->buttons[MTY_CBUTTON_Y] = b & XINPUT_GAMEPAD_Y;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;
	c->buttons[MTY_CBUTTON_BACK] = b & XINPUT_GAMEPAD_BACK;
	c->buttons[MTY_CBUTTON_START] = b & XINPUT_GAMEPAD_START;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = b & XINPUT_GAMEPAD_LEFT_THUMB;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = b & XINPUT_GAMEPAD_RIGHT_THUMB;
	c->buttons[MTY_CBUTTON_DPAD_UP] = b & XINPUT_GAMEPAD_DPAD_UP;
	c->buttons[MTY_CBUTTON_DPAD_DOWN] = b & XINPUT_GAMEPAD_DPAD_DOWN;
	c->buttons[MTY_CBUTTON_DPAD_LEFT] = b & XINPUT_GAMEPAD_DPAD_LEFT;
	c->buttons[MTY_CBUTTON_DPAD_RIGHT] = b & XINPUT_GAMEPAD_DPAD_RIGHT;

	c->values[MTY_CVALUE_THUMB_LX].data = xstate->Gamepad.sThumbLX;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_LY].data = xstate->Gamepad.sThumbLY;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = xstate->Gamepad.sThumbRX;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RY].data = xstate->Gamepad.sThumbRY;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = xstate->Gamepad.bLeftTrigger;
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = xstate->Gamepad.bRightTrigger;
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;
}

static void hid_xinput_state(struct xip *xip, MTY_Window window, MTY_MsgFunc func, void *opaque)
{
	for (uint8_t x = 0; x < 4; x++) {
		if (!xip->disabled) {
			MTY_Msg wmsg = {0};
			wmsg.window = window;
			wmsg.controller.driver = MTY_HID_DRIVER_XINPUT;
			wmsg.controller.numButtons = 14;
			wmsg.controller.numValues = 6;
			wmsg.controller.id = x;

			XINPUT_STATE xstate;
			if (_XInputGetState(x, &xstate) == ERROR_SUCCESS) {
				if (!xip->was_enabled) {
					xip->bbi.vid = 0x045E;
					xip->bbi.pid = 0x0000;

					if (_XInputGetBaseBusInformation)
						_XInputGetBaseBusInformation(x, &xip->bbi);

					xip->was_enabled = true;
				}

				if (xstate.dwPacketNumber != xip->packet) {
					wmsg.type = MTY_WINDOW_MSG_CONTROLLER;
					wmsg.controller.vid = xip->bbi.vid;
					wmsg.controller.pid = xip->bbi.pid;

					hid_xinput_to_mty(&xstate, &wmsg);
					func(&wmsg, opaque);

					xip->packet = xstate.dwPacketNumber;
				}
			} else {
				xip->disabled = true;

				if (xip->was_enabled) {
					wmsg.type = MTY_WINDOW_MSG_DISCONNECT;
					func(&wmsg, opaque);

					xip->was_enabled = false;
				}
			}
		}
	}
}
