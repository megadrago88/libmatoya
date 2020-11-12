// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>

struct hid {
	IOHIDManagerRef mgr;
};

static void hid_destroy(struct hid **hid)
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

	MTY_Free(ctx);
	*hid = NULL;
}

static void hid_write(void *device, const void *buf, size_t size)
{
	const uint8_t *buf8 = buf;

	if (buf8[0] == 0) {
		buf += 1;
		size -= 1;
	}

	IOHIDDeviceSetReport((IOHIDDeviceRef) device, kIOHIDReportTypeOutput, buf8[0], buf, size);
}

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

static void hid_report(void *context, IOReturn result, void *sender, IOHIDReportType type,
	uint32_t reportID, uint8_t *report, CFIndex reportLength)
{
	struct hid *ctx = context;
	IOHIDDeviceRef device = sender;

	int32_t vid = hid_device_get_prop_int(device, CFSTR(kIOHIDVendorIDKey));
	int32_t pid = hid_device_get_prop_int(device, CFSTR(kIOHIDProductIDKey));
}

static struct hid *hid_create(void)
{
	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	bool r = true;

	ctx->mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (!ctx->mgr) {
		r = false;
		MTY_Log("'IOHIDManagerCreate' failed");
		goto except;
	}

	IOHIDManagerSetDeviceMatching(ctx->mgr, NULL);
	IOHIDManagerScheduleWithRunLoop(ctx->mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	IOReturn e = IOHIDManagerOpen(ctx->mgr, kIOHIDOptionsTypeNone);
	if (e != kIOReturnSuccess) {
		r = false;
		MTY_Log("'IOHIDManagerOpen' failed with error 0x%X", e);
		goto except;
	}

	IOHIDManagerRegisterInputReportCallback(ctx->mgr, hid_report, ctx);

	except:

	if (!r)
		hid_destroy(&ctx);

	return ctx;
}
