// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include <xinput.h>

typedef struct {
	WORD vid;
	WORD pid;
	uint8_t _pad[64];
} XINPUT_BASE_BUS_INFORMATION;

struct xip {
	struct xip_state {
		bool disabled;
		bool was_enabled;
		DWORD packet;
		XINPUT_BASE_BUS_INFORMATION bbi;
	} state[4];

	HMODULE xinput1_4;
	DWORD (WINAPI *XInputGetState)(DWORD dwUserIndex, XINPUT_STATE *pState);
	DWORD (WINAPI *XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
	DWORD (WINAPI *XInputGetBaseBusInformation)(DWORD dwUserIndex, XINPUT_BASE_BUS_INFORMATION *pBusInformation);
};

static struct xip *xip_create(void)
{
	struct xip *ctx = MTY_Alloc(1, sizeof(struct xip));

	bool fallback = false;

	ctx->XInputGetState = XInputGetState;
	ctx->XInputSetState = XInputSetState;

	ctx->xinput1_4 = LoadLibrary(L"xinput1_4.dll");
	if (ctx->xinput1_4) {
		ctx->XInputGetState = (void *) GetProcAddress(ctx->xinput1_4, (LPCSTR) 100);
		if (!ctx->XInputGetState) {
			fallback = true;
			ctx->XInputGetState = XInputGetState;
		}

		ctx->XInputSetState = (void *) GetProcAddress(ctx->xinput1_4, "XInputSetState");
		if (!ctx->XInputSetState) {
			fallback = true;
			ctx->XInputSetState = XInputSetState;
		}

		ctx->XInputGetBaseBusInformation = (void *) GetProcAddress(ctx->xinput1_4, (LPCSTR) 104);
	}

	if (!ctx->xinput1_4 || !ctx->XInputGetBaseBusInformation || fallback)
		MTY_Log("Failed to load full XInput 1.4 interface");

	return ctx;
}

static void xip_destroy(struct xip **xip)
{
	if (!xip || !*xip)
		return;

	struct xip *ctx = *xip;

	if (ctx->xinput1_4)
		FreeLibrary(ctx->xinput1_4);

	MTY_Free(ctx);
	*xip = NULL;
}

static void xip_device_arival(struct xip *ctx)
{
	for (uint8_t x = 0; x < 4; x++)
		ctx->state[x].disabled = false;
}

static void xip_to_mty(const XINPUT_STATE *xstate, MTY_Event *evt)
{
	WORD b = xstate->Gamepad.wButtons;
	MTY_ControllerEvent *c = &evt->controller;

	c->buttons[MTY_CBUTTON_X] = b & XINPUT_GAMEPAD_X;
	c->buttons[MTY_CBUTTON_A] = b & XINPUT_GAMEPAD_A;
	c->buttons[MTY_CBUTTON_B] = b & XINPUT_GAMEPAD_B;
	c->buttons[MTY_CBUTTON_Y] = b & XINPUT_GAMEPAD_Y;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = xstate->Gamepad.bLeftTrigger > 0;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = xstate->Gamepad.bRightTrigger > 0;
	c->buttons[MTY_CBUTTON_BACK] = b & XINPUT_GAMEPAD_BACK;
	c->buttons[MTY_CBUTTON_START] = b & XINPUT_GAMEPAD_START;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = b & XINPUT_GAMEPAD_LEFT_THUMB;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = b & XINPUT_GAMEPAD_RIGHT_THUMB;
	c->buttons[MTY_CBUTTON_GUIDE] = b & 0x0400;

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

	bool up = b & XINPUT_GAMEPAD_DPAD_UP;
	bool down = b & XINPUT_GAMEPAD_DPAD_DOWN;
	bool left = b & XINPUT_GAMEPAD_DPAD_LEFT;
	bool right = b & XINPUT_GAMEPAD_DPAD_RIGHT;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}

static void xip_rumble(struct xip *ctx, uint32_t id, uint16_t low, uint16_t high)
{
	if (id >= 4)
		return;

	if (!ctx->state[id].disabled) {
		XINPUT_VIBRATION vb = {0};
		vb.wLeftMotorSpeed = low;
		vb.wRightMotorSpeed = high;

		DWORD e = ctx->XInputSetState(id, &vb);
		if (e != ERROR_SUCCESS)
			MTY_Log("'XInputSetState' failed with error 0x%X", e);
	}
}

static void xip_state(struct xip *ctx, MTY_EventFunc func, void *opaque)
{
	for (uint8_t x = 0; x < 4; x++) {
		struct xip_state *state = &ctx->state[x];

		if (!state->disabled) {
			MTY_Event evt = {0};
			evt.controller.type = MTY_CTYPE_XINPUT;
			evt.controller.numButtons = 13;
			evt.controller.numValues = 7;
			evt.controller.id = x;

			XINPUT_STATE xstate;
			if (ctx->XInputGetState(x, &xstate) == ERROR_SUCCESS) {
				if (!state->was_enabled) {
					state->bbi.vid = 0x045E;
					state->bbi.pid = 0x0000;

					if (ctx->XInputGetBaseBusInformation) {
						DWORD e = ctx->XInputGetBaseBusInformation(x, &state->bbi);
						if (e != ERROR_SUCCESS)
							MTY_Log("'XInputGetBaseBusInformation' failed with error 0x%X", e);
					}

					state->was_enabled = true;
				}

				if (xstate.dwPacketNumber != state->packet) {
					evt.type = MTY_EVENT_CONTROLLER;
					evt.controller.vid = state->bbi.vid;
					evt.controller.pid = state->bbi.pid;

					xip_to_mty(&xstate, &evt);
					func(&evt, opaque);

					state->packet = xstate.dwPacketNumber;
				}
			} else {
				state->disabled = true;

				if (state->was_enabled) {
					evt.type = MTY_EVENT_DISCONNECT;
					func(&evt, opaque);

					state->was_enabled = false;
				}
			}
		}
	}
}
