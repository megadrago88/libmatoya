// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "hid/hid.h"

#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>

#include "matoya.h"


// HID

struct hid {
	uint32_t id;
	MTY_Hash *devices;
	MTY_Hash *devices_rev;
	IOHIDManagerRef mgr;
	HID_CONNECT connect;
	HID_DISCONNECT disconnect;
	HID_REPORT report;
	void *opaque;
};

struct hdevice {
	IOHIDDeviceRef device;
	void *state;
	uint32_t id;
	uint16_t vid;
	uint16_t pid;
};

static int32_t hid_device_get_prop_int(IOHIDDeviceRef device, CFStringRef key)
{
	CFNumberRef num = IOHIDDeviceGetProperty(device, key);
	if (!num)
		return 0;

	int32_t i = 0;
	if (!CFNumberGetValue(num, kCFNumberSInt32Type, &i))
		return 0;

	return i;
}

static struct hdevice *hid_device_create(void *native_device)
{
	struct hdevice *ctx = MTY_Alloc(1, sizeof(struct hdevice));
	ctx->device = (IOHIDDeviceRef) native_device;
	ctx->vid = hid_device_get_prop_int(ctx->device, CFSTR(kIOHIDVendorIDKey));
	ctx->pid = hid_device_get_prop_int(ctx->device, CFSTR(kIOHIDProductIDKey));
	ctx->state = MTY_Alloc(HID_STATE_MAX, 1);

	return ctx;
}

static void hid_device_destroy(void *hdevice)
{
	if (!hdevice)
		return;

	struct hdevice *ctx = hdevice;

	MTY_Free(ctx->state);
	MTY_Free(ctx);
}

static void hid_report(void *context, IOReturn result, void *sender, IOHIDReportType type,
	uint32_t reportID, uint8_t *report, CFIndex reportLength)
{
	struct hid *ctx = context;

	struct hdevice *dev = MTY_HashGetInt(ctx->devices, (intptr_t) sender);
	if (!dev) {
		dev = hid_device_create(sender);
		dev->id = ++ctx->id;
		MTY_HashSetInt(ctx->devices, (intptr_t) sender, dev);
		MTY_HashSetInt(ctx->devices_rev, dev->id, dev);

		ctx->connect(dev, ctx->opaque);
	}

	ctx->report(dev, report, reportLength, ctx->opaque);
}

static void hid_disconnect(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
	struct hid *ctx = context;

	struct hdevice *dev = MTY_HashPopInt(ctx->devices, (intptr_t) device);
	if (dev) {
		ctx->disconnect(dev, ctx->opaque);

		MTY_HashPopInt(ctx->devices_rev, dev->id);
		hid_device_destroy(dev);
	}
}

static CFMutableDictionaryRef hid_match_dict(uint16_t usage_page, uint16_t usage)
{
	CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, kIOHIDOptionsTypeNone,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	CFNumberRef v = CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &usage_page);
	CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsagePageKey), v);
	CFRelease(v);

	v = CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &usage);
	CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsageKey), v);
	CFRelease(v);

	return dict;
}

struct hid *hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque)
{
	bool r = true;

	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	ctx->connect = connect;
	ctx->disconnect = disconnect;
	ctx->report = report;
	ctx->opaque = opaque;
	ctx->devices = MTY_HashCreate(0);
	ctx->devices_rev = MTY_HashCreate(0);

	ctx->mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (!ctx->mgr) {
		r = false;
		MTY_Log("'IOHIDManagerCreate' failed");
		goto except;
	}

	CFMutableDictionaryRef d0 = hid_match_dict(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);
	CFMutableDictionaryRef d1 = hid_match_dict(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
	CFMutableDictionaryRef d2 = hid_match_dict(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController);
	CFMutableDictionaryRef dict_list[] = {d0, d1, d2};

	CFArrayRef matches = CFArrayCreate(kCFAllocatorDefault, (const void **) dict_list, 3, NULL);
	IOHIDManagerSetDeviceMatchingMultiple(ctx->mgr, matches);

	IOHIDManagerScheduleWithRunLoop(ctx->mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	IOReturn e = IOHIDManagerOpen(ctx->mgr, kIOHIDOptionsTypeNone);
	if (e != kIOReturnSuccess) {
		r = false;
		MTY_Log("'IOHIDManagerOpen' failed with error 0x%X", e);
		goto except;
	}

	IOHIDManagerRegisterInputReportCallback(ctx->mgr, hid_report, ctx);
	IOHIDManagerRegisterDeviceRemovalCallback(ctx->mgr, hid_disconnect, ctx);

	except:

	if (!r)
		hid_destroy(&ctx);

	return ctx;
}

struct hdevice *hid_get_device_by_id(struct hid *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->devices_rev, id);
}

void hid_destroy(struct hid **hid)
{
	if (!hid || !*hid)
		return;

	struct hid *ctx = *hid;

	if (ctx->mgr) {
		IOReturn e = IOHIDManagerClose(ctx->mgr, kIOHIDOptionsTypeNone);
		if (e != kIOReturnSuccess)
			MTY_Log("'IOHIDManagerClose' failed with error 0x%X", e);

		CFRelease(ctx->mgr);
	}

	MTY_HashDestroy(&ctx->devices, hid_device_destroy);
	MTY_HashDestroy(&ctx->devices_rev, NULL);

	MTY_Free(ctx);
	*hid = NULL;
}


void hid_device_write(struct hdevice *ctx, const void *buf, size_t size)
{
	const uint8_t *buf8 = buf;

	if (buf8[0] == 0) {
		buf += 1;
		size -= 1;
	}

	IOReturn e = IOHIDDeviceSetReport(ctx->device, kIOHIDReportTypeOutput, buf8[0], buf, size);
	if (e != kIOReturnSuccess)
		MTY_Log("'IOHIDDeviceSetReport' failed with error 0x%X", e);
}

bool hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out)
{
	const uint8_t *buf8 = buf;

	if (buf8[0] == 0) {
		buf += 1;
		size -= 1;
	}

	CFIndex out = size;
	IOReturn e = IOHIDDeviceGetReport(ctx->device, kIOHIDReportTypeFeature, buf8[0], buf, &out);
	if (e == kIOReturnSuccess) {
		*size_out = out;

		if (buf8[0] == 0)
			(*size_out)++;

		return true;

	} else {
		MTY_Log("'IOHIDDeviceGetReport' failed with error 0x%X", e);
	}

	return false;
}

void *hid_device_get_state(struct hdevice *ctx)
{
	return ctx->state;
}

uint16_t hid_device_get_vid(struct hdevice *ctx)
{
	return ctx->vid;
}

uint16_t hid_device_get_pid(struct hdevice *ctx)
{
	return ctx->pid;
}

uint32_t hid_device_get_id(struct hdevice *ctx)
{
	return ctx->id;
}
