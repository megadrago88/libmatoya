// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "hid/hid.h"

#include "hidpi.h"

struct hid {
	uint32_t id;
	MTY_Hash *devices;
	MTY_Hash *devices_rev;
	HID_CONNECT connect;
	HID_DISCONNECT disconnect;
	HID_REPORT report;
	void *opaque;
};

struct hdevice {
	RID_DEVICE_INFO di;
	PHIDP_PREPARSED_DATA ppd;
	HIDP_CAPS caps;
	HIDP_BUTTON_CAPS *bcaps;
	HIDP_VALUE_CAPS *vcaps;
	wchar_t *name;
	void *state;
	uint32_t id;
	uint16_t vid;
	uint16_t pid;
};

static void hid_device_destroy(void *opaque)
{
	if (!opaque)
		return;

	struct hdevice *ctx = opaque;

	MTY_Free(ctx->bcaps);
	MTY_Free(ctx->vcaps);
	MTY_Free(ctx->ppd);
	MTY_Free(ctx->name);
	MTY_Free(ctx->driver_state);
	MTY_Free(ctx);
}

static struct hdevice *hid_device_create(HANDLE device)
{
	struct hdevice *ctx = MTY_Alloc(1, sizeof(struct hdevice));

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

	ctx->is_xinput = wcsstr(ctx->name, L"IG_");
	ctx->state = MTY_Alloc(MTY_HID_DRIVER_STATE_MAX, 1);

	except:

	if (!r) {
		hid_destroy(ctx);
		ctx = NULL;
	}

	return ctx;
}


struct hid *hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque)
{
	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	ctx->id = 32;
	ctx->devices = MTY_HashCreate(0);
	ctx->devices_rev = MTY_HashCreate(0);
	ctx->connect = conntext;
	ctx->disconnect = disconnect;
	ctx->report = report;
	ctx->opaque = opaque;
}

struct hdevice *hid_get_device_by_id(struct hid *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->devices_rev, id);
}

void hid_win32_feed_report(struct hid *ctx, void *device, const void *buf, size_t size)
{
}

void hid_win32_device_change(struct hid *ctx, intptr_t wparam, intptr_t lparam)
{
	if (wparam == GIDC_ARRIVAL) {
		struct hdevice *dev = hid_device_create((HANDLE) lparam);
		if (dev)
			dev->id = ctx->id++;
			hid_destroy(MTY_HashSetInt(ctx->devices, lparam, dev));
			MTY_HashSetInt(ctx->devices_rev, dev->id, dev);
			ctx->connect(dev, ctx->opaque);
		}

	} else if (wparam == GIDC_REMOVAL) {
		struct hdevice *dev = MTY_HashPopInt(ctx->devices, lparam);
		if (dev) {
			ctx->disconnect(dev, ctx->opaque);

			MTY_HashPopInt(ctx->devices_rev, dev->id);
			hid_device_destroy(dev);
		}
	}
}

void hid_win32_listen(void *hwnd)
{
	RAWINPUTDEVICE rid[3] = {
		{0x01, 0x04, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
		{0x01, 0x05, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
		{0x01, 0x08, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
	};

	if (!RegisterRawInputDevices(rid, 3, sizeof(RAWINPUTDEVICE)))
		MTY_Log("'RegisterRawInputDevices' failed with error 0x%X", GetLastError());
}

void hid_destroy(struct hid **hid)
{
	if (!hid || !*hid)
		return;

	struct hid *ctx = *hid;

	MTY_HashDestroy(&ctx->devices, hid_device_destroy);
	MTY_HashDestroy(&ctx->devices_rev, NULL);

	MTY_Free(ctx);
	*hid = NULL;
}

void hid_device_write(struct hdevice *ctx, const void *buf, size_t size)
{
	OVERLAPPED ov = {0};
	ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE device = CreateFile(ctx->name, GENERIC_WRITE | GENERIC_READ,
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
		if (!GetOverlappedResult(device, &ov, &written, FALSE)) {
			MTY_Log("'GetOverlappedResult' failed with error 0x%X", GetLastError());
			goto except;
		}
	}

	except:

	if (ov.hEvent)
		CloseHandle(ov.hEvent);

	if (device)
		CloseHandle(device);
}

bool hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out)
{
	bool r = true;

	*size_out = 0;
	OVERLAPPED ov = {0};

	HANDLE device = CreateFile(ctx->name, GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, NULL);

	if (!device) {
		r = false;
		MTY_Log("'CreateFile' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!DeviceIoControl(device, IOCTL_HID_GET_FEATURE, buf, size, buf, size, size_out, &ov)) {
		DWORD e = GetLastError();
		if (e != ERROR_IO_PENDING) {
			r = false;
			MTY_Log("'DeviceIoControl' failed with error 0x%X", e);
			goto except;
		}
	}

	if (!GetOverlappedResult(device, &ov, size_out, TRUE)) {
		r = false;
		MTY_Log("'GetOverlappedResult' failed with error 0x%X", GetLastError());
		goto except;
	}

	(*size_out)++;

	except:

	if (device)
		CloseHandle(device);

	return r;
}

void *hid_device_get_state(struct hdevice *ctx)
{
	return ctx->state;
}

uint16_t hid_device_get_vid(struct hdevice *ctx)
{
	return (uint16_t) ctx->di.hid.dwVendorId;
}

uint16_t hid_device_get_pid(struct hdevice *ctx)
{
	return (uint16_t) ctx->di.hid.dwProductId;
}

uint32_t hid_device_get_id(struct hdevice *ctx)
{
	return ctx->id;
}

void hid_default_state(struct hdevice *ctx, const void *data, size_t dsize, MTY_Msg *wmsg)
{
	MTY_Controller *c = &wmsg->controller;

	// Buttons
	for (uint32_t x = 0; x < ctx->caps.NumberInputButtonCaps; x++) {
		const HIDP_BUTTON_CAPS *bcap = &ctx->bcaps[x];

		// Should we consider usages other than 0x09 (Button)?
		if (bcap->UsagePage == 0x09) {
			c->numButtons += (uint8_t) (bcap->Range.UsageMax - bcap->Range.UsageMin + 1);

			ULONG n = MTY_CBUTTON_MAX;
			USAGE usages[MTY_CBUTTON_MAX];
			NTSTATUS e = HidP_GetUsages(HidP_Input, bcap->UsagePage, 0, usages, &n, ctx->ppd, data, dsize);
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
	for (uint32_t x = 0; x < ctx->caps.NumberInputValueCaps && c->numValues < MTY_CVALUE_MAX; x++) {
		const HIDP_VALUE_CAPS *vcap = &ctx->vcaps[x];

		ULONG value = 0;
		NTSTATUS e = HidP_GetUsageValue(HidP_Input, vcap->UsagePage, 0, vcap->Range.UsageMin, &value, ctx->ppd, data, dsize);
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
	c->vid = (uint16_t) ctx->di.hid.dwVendorId;
	c->pid = (uint16_t) ctx->di.hid.dwProductId;
	c->driver = MTY_HID_DRIVER_DEFAULT;
	c->id = ctx->id;
}
