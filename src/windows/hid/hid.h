// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define MTY_HID_DRIVER_STATE_MAX 1024


// Windows HID Parser

typedef USHORT USAGE;
typedef USAGE *PUSAGE;
typedef struct _HIDP_PREPARSED_DATA *PHIDP_PREPARSED_DATA;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

typedef enum _HIDP_REPORT_TYPE {
	HidP_Input,
	HidP_Output,
	HidP_Feature
} HIDP_REPORT_TYPE;

typedef struct _HIDP_CAPS {
	USAGE Usage;
	USAGE UsagePage;
	USHORT InputReportByteLength;
	USHORT OutputReportByteLength;
	USHORT FeatureReportByteLength;
	USHORT Reserved[17];

	USHORT NumberLinkCollectionNodes;

	USHORT NumberInputButtonCaps;
	USHORT NumberInputValueCaps;
	USHORT NumberInputDataIndices;

	USHORT NumberOutputButtonCaps;
	USHORT NumberOutputValueCaps;
	USHORT NumberOutputDataIndices;

	USHORT NumberFeatureButtonCaps;
	USHORT NumberFeatureValueCaps;
	USHORT NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

typedef struct _HIDP_BUTTON_CAPS {
	USAGE UsagePage;
	UCHAR ReportID;
	BOOLEAN IsAlias;

	USHORT BitField;
	USHORT LinkCollection;

	USAGE LinkUsage;
	USAGE LinkUsagePage;

	BOOLEAN IsRange;
	BOOLEAN IsStringRange;
	BOOLEAN IsDesignatorRange;
	BOOLEAN IsAbsolute;

	ULONG Reserved[10];
	union {
		struct {
			USAGE UsageMin, UsageMax;
			USHORT StringMin, StringMax;
			USHORT DesignatorMin, DesignatorMax;
			USHORT DataIndexMin, DataIndexMax;
		} Range;

		struct  {
			USAGE Usage, Reserved1;
			USHORT StringIndex, Reserved2;
			USHORT DesignatorIndex, Reserved3;
			USHORT DataIndex, Reserved4;
		} NotRange;
	};
} HIDP_BUTTON_CAPS, *PHIDP_BUTTON_CAPS;

typedef struct _HIDP_VALUE_CAPS {
	USAGE UsagePage;
	UCHAR ReportID;
	BOOLEAN IsAlias;

	USHORT BitField;
	USHORT LinkCollection;

	USAGE LinkUsage;
	USAGE LinkUsagePage;

	BOOLEAN IsRange;
	BOOLEAN IsStringRange;
	BOOLEAN IsDesignatorRange;
	BOOLEAN IsAbsolute;

	BOOLEAN HasNull;
	UCHAR Reserved;
	USHORT BitSize;

	USHORT ReportCount;
	USHORT Reserved2[5];

	ULONG UnitsExp;
	ULONG Units;

	LONG LogicalMin, LogicalMax;
	LONG PhysicalMin, PhysicalMax;

	union {
		struct {
			USAGE UsageMin, UsageMax;
			USHORT StringMin, StringMax;
			USHORT DesignatorMin, DesignatorMax;
			USHORT DataIndexMin, DataIndexMax;
		} Range;

		struct {
			USAGE Usage, Reserved1;
			USHORT StringIndex, Reserved2;
			USHORT DesignatorIndex, Reserved3;
			USHORT DataIndex, Reserved4;
		} NotRange;
	};
} HIDP_VALUE_CAPS, *PHIDP_VALUE_CAPS;

#define FACILITY_HID_ERROR_CODE 0x11

#define HIDP_ERROR_CODES(SEV, CODE) \
		((NTSTATUS) (((SEV) << 28) | (FACILITY_HID_ERROR_CODE << 16) | (CODE)))

#define HIDP_STATUS_SUCCESS                 (HIDP_ERROR_CODES(0x0,0))
#define HIDP_STATUS_NULL                    (HIDP_ERROR_CODES(0x8,1))
#define HIDP_STATUS_INVALID_PREPARSED_DATA  (HIDP_ERROR_CODES(0xC,1))
#define HIDP_STATUS_INVALID_REPORT_TYPE     (HIDP_ERROR_CODES(0xC,2))
#define HIDP_STATUS_INVALID_REPORT_LENGTH   (HIDP_ERROR_CODES(0xC,3))
#define HIDP_STATUS_USAGE_NOT_FOUND         (HIDP_ERROR_CODES(0xC,4))
#define HIDP_STATUS_VALUE_OUT_OF_RANGE      (HIDP_ERROR_CODES(0xC,5))
#define HIDP_STATUS_BAD_LOG_PHY_VALUES      (HIDP_ERROR_CODES(0xC,6))
#define HIDP_STATUS_BUFFER_TOO_SMALL        (HIDP_ERROR_CODES(0xC,7))
#define HIDP_STATUS_INTERNAL_ERROR          (HIDP_ERROR_CODES(0xC,8))
#define HIDP_STATUS_I8042_TRANS_UNKNOWN     (HIDP_ERROR_CODES(0xC,9))
#define HIDP_STATUS_INCOMPATIBLE_REPORT_ID  (HIDP_ERROR_CODES(0xC,0xA))
#define HIDP_STATUS_NOT_VALUE_ARRAY         (HIDP_ERROR_CODES(0xC,0xB))
#define HIDP_STATUS_IS_VALUE_ARRAY          (HIDP_ERROR_CODES(0xC,0xC))
#define HIDP_STATUS_DATA_INDEX_NOT_FOUND    (HIDP_ERROR_CODES(0xC,0xD))
#define HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE (HIDP_ERROR_CODES(0xC,0xE))
#define HIDP_STATUS_BUTTON_NOT_PRESSED      (HIDP_ERROR_CODES(0xC,0xF))
#define HIDP_STATUS_REPORT_DOES_NOT_EXIST   (HIDP_ERROR_CODES(0xC,0x10))
#define HIDP_STATUS_NOT_IMPLEMENTED         (HIDP_ERROR_CODES(0xC,0x20))

NTSTATUS __stdcall HidP_GetCaps(PHIDP_PREPARSED_DATA PreparsedData, PHIDP_CAPS Capabilities);
NTSTATUS __stdcall HidP_GetButtonCaps(HIDP_REPORT_TYPE ReportType, PHIDP_BUTTON_CAPS ButtonCaps,
	PUSHORT ButtonCapsLength, PHIDP_PREPARSED_DATA PreparsedData);
NTSTATUS __stdcall HidP_GetValueCaps(HIDP_REPORT_TYPE ReportType, PHIDP_VALUE_CAPS ValueCaps,
	PUSHORT ValueCapsLength, PHIDP_PREPARSED_DATA PreparsedData);
NTSTATUS __stdcall HidP_GetUsages(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
	PUSAGE UsageList, PULONG UsageLength, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength);
NTSTATUS __stdcall HidP_GetUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
	USAGE Usage, PULONG UsageValue, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength);


// IO

static void hid_write(wchar_t *name, void *buf, DWORD size)
{
	OVERLAPPED ov = {0};
	ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE device = CreateFile(name, GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, NULL);

	if (!device) {
		MTY_Log("'CreateFile' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!WriteFile(device, buf, size, NULL, &ov)) {
		DWORD e = GetLastError();
		if (e != ERROR_IO_PENDING) {
			MTY_Log("'WriteFile' failed with error 0x%X", e);
			goto except;
		}
	}

	if (WaitForSingleObject(ov.hEvent, 1000) == WAIT_OBJECT_0) {
		DWORD written = 0;
		GetOverlappedResult(device, &ov, &written, FALSE);
	}

	except:

	if (ov.hEvent)
		CloseHandle(ov.hEvent);

	if (device)
		CloseHandle(device);
}


// Drivers

struct hid {
	RID_DEVICE_INFO di;
	PHIDP_PREPARSED_DATA ppd;
	HIDP_CAPS caps;
	HIDP_BUTTON_CAPS *bcaps;
	HIDP_VALUE_CAPS *vcaps;
	MTY_HIDDriver driver;
	void *driver_state;
	wchar_t *name;
	uint32_t id;
};

#include "default.h"
#include "xinput.h"
#include "ps4.h"
#include "nx.h"

static uint32_t HID_ID = 32;

static MTY_HIDDriver hid_get_driver(uint16_t vid, uint16_t pid)
{
	uint32_t id = ((uint32_t) vid << 16) | pid;

	switch (id) {
		// Switch
		case 0x057E2009: // Nintendo Switch Pro
		case 0x057E2006: // Nintendo Switch Joycon
		case 0x057E2007: // Nintendo Switch Joycon
		case 0x057E2017: // Nintendo Switch SNES Controller
		// case 0x0F0D00C1: // HORIPAD for Nintendo Switch
		// case 0x0F0D0092: // HORI Pokken Tournament DX Pro Pad
		case 0x0F0D00F6: // HORI Wireless Switch Pad
		// case 0x0E6F0185: // PDP Wired Fight Pad Pro for Nintendo Switch
		// case 0x0E6F0180: // PDP Faceoff Wired Pro Controller for Nintendo Switch
		// case 0x0E6F0181: // PDP Faceoff Deluxe Wired Pro Controller for Nintendo Switch
		// case 0x20D6A711: // PowerA Wired Controller Plus/PowerA Wired Controller Nintendo GameCube Style
		// case 0x20D6A712: // PowerA - Fusion Fight Pad
		// case 0x20D6A713: // PowerA - Super Mario Controller
			return MTY_HID_DRIVER_SWITCH;

		// PS4
		case 0x054C05C4: // Sony DualShock 4 Gen1
		case 0x054C09CC: // Sony DualShock 4 Gen2
		case 0x054C0BA0: // Sony PS4 Controller (Wireless dongle)
		case 0x0079181B: // Venom Arcade Stick - XXX:this may not work and may need to be called a ps3 controller
		case 0x054C05C5: // STRIKEPAD PS4 Grip Add-on
		case 0x07388250: // Mad Catz FightPad Pro PS4
		case 0x07388384: // Mad Catz FightStick TE S+ PS4
		case 0x07388480: // Mad Catz FightStick TE 2 PS4
		case 0x07388481: // Mad Catz FightStick TE 2+ PS4
		case 0x0C120E10: // Armor Armor 3 Pad PS4
		case 0x0C121CF6: // EMIO PS4 Elite Controller
		case 0x0C120E15: // Game:Pad 4
		case 0x0C120EF6: // Hitbox Arcade Stick
		case 0x0F0D0055: // HORIPAD 4 FPS
		case 0x0F0D0066: // HORIPAD 4 FPS Plus
		case 0x0F0D0084: // HORI Fighting Commander PS4
		case 0x0F0D008A: // HORI Real Arcade Pro 4
		case 0x0F0D009C: // HORI TAC PRO mousething
		case 0x0F0D00A0: // HORI TAC4 mousething
		case 0x0F0D00EE: // Hori mini wired https://www.playstation.com/en-us/explore/accessories/gaming-controllers/mini-wired-gamepad/
		case 0x11C04001: // "PS4 Fun Controller" added from user log
		case 0x146B0D01: // Nacon Revolution Pro Controller - has gyro
		case 0x146B0D02: // Nacon Revolution Pro Controller v2 - has gyro
		case 0x146B0D10: // NACON Revolution Infinite - has gyro
		case 0x15320401: // Razer Panthera PS4 Controller
		case 0x15321000: // Razer Raiju PS4 Controller
		case 0x15321004: // Razer Raiju 2 Ultimate USB
		case 0x15321007: // Razer Raiju 2 Tournament edition USB
		case 0x15321008: // Razer Panthera Evo Fightstick
		case 0x15321009: // Razer Raiju 2 Ultimate BT
		case 0x1532100A: // Razer Raiju 2 Tournament edition BT
		case 0x15321100: // Razer RAION Fightpad - Trackpad, no gyro, lightbar hardcoded to green
		case 0x20D6792A: // PowerA - Fusion Fight Pad
		case 0x75450104: // Armor 3 or Level Up Cobra - At least one variant has gyro
		case 0x98860025: // Astro C40
		case 0x0E6F0207: // Victrix Pro Fightstick w/ Touchpad for PS4
			return MTY_HID_DRIVER_PS4;
	}

	return MTY_HID_DRIVER_DEFAULT;
}

static void hid_init(struct hid *hid)
{
	switch (hid->driver) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_init(hid);
			break;
	}
}

static void hid_state(struct hid *hid, void *data, ULONG dsize, MTY_Msg *wmsg)
{
	switch (hid->driver) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_state(hid, data, dsize, wmsg);
			break;
		case MTY_HID_DRIVER_PS4:
			hid_ps4_state(hid, data, dsize, wmsg);
			break;
		case MTY_HID_DRIVER_DEFAULT:
			hid_default_state(hid, data, dsize, wmsg);
			break;

		// On Windows, XInput is handled by the canonical XInput API
		case MTY_HID_DRIVER_XINPUT:
			break;
	}
}

static void hid_rumble(struct hid *hid, uint16_t low, uint16_t high)
{
	switch (hid->driver) {
		case MTY_HID_DRIVER_SWITCH:
			hid_nx_rumble(hid, low > 0, high > 0);
			break;
		case MTY_HID_DRIVER_PS4:
			break;
		case MTY_HID_DRIVER_DEFAULT:
			break;

		// On Windows, XInput is handled by the canonical XInput API by player index (0-3)
		case MTY_HID_DRIVER_XINPUT:
			break;
	}
}


// Public

static void hid_destroy(void *opaque)
{
	if (!opaque)
		return;

	struct hid *ctx = opaque;

	MTY_Free(ctx->bcaps);
	MTY_Free(ctx->vcaps);
	MTY_Free(ctx->ppd);
	MTY_Free(ctx->name);
	MTY_Free(ctx->driver_state);
	MTY_Free(ctx);
}

static struct hid *hid_create(HANDLE device)
{
	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));

	bool r = true;

	UINT size = 0;
	UINT e = GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, NULL, &size);
	if (e != 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->ppd = MTY_Alloc(size, 1);
	e = GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, ctx->ppd, &size);
	if (e == 0xFFFFFFFF || e == 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	NTSTATUS ne = HidP_GetCaps(ctx->ppd, &ctx->caps);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->bcaps = MTY_Alloc(ctx->caps.NumberInputButtonCaps, sizeof(HIDP_BUTTON_CAPS));
	ne = HidP_GetButtonCaps(HidP_Input, ctx->bcaps, &ctx->caps.NumberInputButtonCaps, ctx->ppd);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetButtonCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->vcaps = MTY_Alloc(ctx->caps.NumberInputValueCaps, sizeof(HIDP_VALUE_CAPS));
	ne = HidP_GetValueCaps(HidP_Input, ctx->vcaps, &ctx->caps.NumberInputValueCaps, ctx->ppd);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetValueCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->di.cbSize = size = sizeof(RID_DEVICE_INFO);
	e = GetRawInputDeviceInfo(device, RIDI_DEVICEINFO, &ctx->di, &size);
	if (e == 0xFFFFFFFF || e == 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	size = 0;
	e = GetRawInputDeviceInfo(device, RIDI_DEVICENAME, NULL, &size);
	if (e != 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->name = MTY_Alloc(size, sizeof(wchar_t));
	e = GetRawInputDeviceInfo(device, RIDI_DEVICENAME, ctx->name, &size);
	if (e == 0xFFFFFFFF) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->driver = wcsstr(ctx->name, L"IG_") ? MTY_HID_DRIVER_XINPUT :
		hid_get_driver((uint16_t) ctx->di.hid.dwVendorId, (uint16_t) ctx->di.hid.dwProductId);
	ctx->driver_state = MTY_Alloc(MTY_HID_DRIVER_STATE_MAX, 1);
	ctx->id = HID_ID++;

	hid_init(ctx);

	except:

	if (!r) {
		hid_destroy(ctx);
		ctx = NULL;
	}

	return ctx;
}
