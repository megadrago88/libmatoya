// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once


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


// State, Drivers

struct hid {
	RID_DEVICE_INFO di;
	PHIDP_PREPARSED_DATA ppd;
	HIDP_CAPS caps;
	HIDP_BUTTON_CAPS *bcaps;
	HIDP_VALUE_CAPS *vcaps;
	MTY_HIDDriver driver;
	wchar_t *name;
	uint32_t id;
};

static uint32_t HID_ID = 32;


// IO, Driver Detection

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

static MTY_HIDDriver hid_get_driver(uint16_t vid, uint16_t pid)
{
	uint32_t id = ((uint32_t) vid << 16) | pid;

	switch (id) {
		// Switch
		case 0x057E2009: return MTY_HID_DRIVER_SWITCH; // Switch Pro
	}

	return MTY_HID_DRIVER_DEFAULT;
}


// Public

#include "hid-default.h"
#include "hid-switch.h"
#include "hid-ps4.h"

static void hid_destroy(void *opaque)
{
	if (!opaque)
		return;

	struct hid *ctx = opaque;

	MTY_Free(ctx->bcaps);
	MTY_Free(ctx->vcaps);
	MTY_Free(ctx->ppd);
	MTY_Free(ctx->name);
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
	ctx->id = HID_ID++;

	except:

	if (!r) {
		hid_destroy(ctx);
		ctx = NULL;
	}

	return ctx;
}


// State Reports

static void hid_state(struct hid *hid, void *data, ULONG dsize, MTY_Msg *wmsg)
{
	switch (hid->driver) {
		case MTY_HID_DRIVER_SWITCH:
			hid_switch_state(hid, data, dsize, wmsg);
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
