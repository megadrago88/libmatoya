// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "hid/hid.h"

#include <string.h>
#include <stdio.h>

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/joystick.h>

#include "udev-dl.h"

#define HID_FD_MAX     33
#define HID_AXMAP_MAX  ABS_CNT
#define HID_BTNMAP_MAX (KEY_MAX - BTN_MISC + 1)

struct hid {
	bool init_scan;
	struct udev *udev;
	struct udev_monitor *udev_monitor;
	MTY_Hash *devices;
	MTY_Hash *devices_rev;
	HID_CONNECT connect;
	HID_DISCONNECT disconnect;
	HID_REPORT report;
	struct pollfd fds[HID_FD_MAX];
	void *opaque;
};

struct hdevice {
	MTY_Controller state;
	bool gamepad; // PS4, XInput
	uint16_t btnmap[HID_BTNMAP_MAX];
	uint8_t axmap[HID_AXMAP_MAX];
	uint8_t slot;
	uint8_t nbuttons;
	uint8_t naxes;
	uint32_t id;
	uint16_t vid;
	uint16_t pid;
};

static void __attribute__((destructor)) hid_global_destroy(void)
{
	udev_dl_global_destroy();
}

static void hid_device_destroy(void *hdevice)
{
	if (!hdevice)
		return;

	struct hdevice *ctx = hdevice;

	MTY_Free(ctx);
}

static uint8_t hid_find_slot(struct hid *ctx)
{
	for (uint8_t x = 1; x < HID_FD_MAX; x++)
		if (ctx->fds[x].fd == -1)
			return x;

	return 0;
}

static void hid_device_add(struct hid *ctx, const char *devnode)
{
	struct hdevice *hdev = MTY_HashGet(ctx->devices, devnode);
	if (hdev)
		return;

	uint8_t slot = hid_find_slot(ctx);
	if (slot == 0)
		return;

	ctx->fds[slot].fd = open(devnode, O_RDWR | O_NONBLOCK);

	if (ctx->fds[slot].fd >= 0) {
		hdev = MTY_Alloc(1, sizeof(struct hdevice));
		hdev->id = ctx->fds[slot].fd;
		hdev->slot = slot;

		hdev->state.driver = MTY_HID_DRIVER_DEFAULT;
		hdev->state.id = hdev->id;
		hdev->state.vid = 0; // TODO
		hdev->state.pid = 0; // TODO

		ioctl(ctx->fds[slot].fd, JSIOCGBTNMAP, hdev->btnmap);
		ioctl(ctx->fds[slot].fd, JSIOCGAXMAP, hdev->axmap);
		ioctl(ctx->fds[slot].fd, JSIOCGAXES, &hdev->naxes);
		ioctl(ctx->fds[slot].fd, JSIOCGBUTTONS, &hdev->nbuttons);

		// joydev seems to have two different mappings, one for BTN_GAMEPAD (0x130-0x13e)
		// and joystcik BTN_JOYSTICK (0x120-0x12f)
		for (uint8_t x = 0; x < hdev->nbuttons; x++)
			if (hdev->btnmap[x] >= 0x130 && hdev->btnmap[x] < 0x140)
				hdev->gamepad = true;

		MTY_HashSet(ctx->devices, devnode, hdev);
		MTY_HashSetInt(ctx->devices_rev, hdev->id, hdev);

		ctx->connect(hdev, ctx->opaque);

	} else {
		MTY_Log("'open' failed with errno %d", errno);
	}
}

static void hid_device_remove(struct hid *ctx, const char *devnode)
{
	struct hdevice *hdev = MTY_HashPop(ctx->devices, devnode);
	if (!hdev)
		return;

	ctx->disconnect(hdev, ctx->opaque);
	int32_t *fd = &ctx->fds[hdev->slot].fd;

	if (*fd >= 0) {
		close(*fd);
		*fd = -1;
	}

	MTY_HashPopInt(ctx->devices_rev, hdev->id);
	hid_device_destroy(hdev);
}

static void hid_new_device(struct hid *ctx)
{
	struct udev_device *dev = udev_monitor_receive_device(ctx->udev_monitor);
	if (!dev)
		return;

	const char *action = udev_device_get_action(dev);
	const char *devnode = udev_device_get_devnode(dev);
	if (!action || !devnode)
		goto except;

	if (!strstr(devnode, "/js"))
		goto except;

	if (!strcmp(action, "add")) {
		hid_device_add(ctx, devnode);

	} else if (!strcmp(action, "remove")) {
		hid_device_remove(ctx, devnode);
	}

	except:

	udev_device_unref(dev);
}

static void hid_joystick_event(struct hid *ctx, int32_t fd)
{
	struct hdevice *hdev = MTY_HashGetInt(ctx->devices_rev, fd);
	if (!hdev)
		return;

	MTY_Controller *c = &hdev->state;
	struct js_event event = {0};

	if (read(fd, &event, sizeof(struct js_event)) != sizeof(struct js_event))
		return;

	if ((event.type & 0xF) == JS_EVENT_BUTTON) {
		if (event.number >= MTY_CBUTTON_MAX)
			return;

		if (event.number >= c->numButtons)
			c->numButtons = event.number + 1;

		c->buttons[event.number] = event.value ? true : false;

	} else if ((event.type & 0xF) == JS_EVENT_AXIS) {
		if (event.number >= MTY_CVALUE_MAX)
			return;

		if (event.number >= c->numValues)
			c->numValues = event.number + 1;

		c->values[event.number].data = event.value;
		c->values[event.number].min = INT16_MIN;
		c->values[event.number].max = INT16_MAX;
	}

	if (!(event.type & 0x80))
		ctx->report(hdev, NULL, 0, ctx->opaque);
}

static void hid_initial_scan(struct hid *ctx)
{
	struct udev_enumerate *enumerate = udev_enumerate_new(ctx->udev);
	if (!enumerate)
		return;

	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devs = udev_enumerate_get_list_entry(enumerate);
	for (struct udev_list_entry *item = devs; item; item = udev_list_entry_get_next(item)) {
		const char *name = udev_list_entry_get_name(item);
		struct udev_device *dev = udev_device_new_from_syspath(ctx->udev, name);
		if (dev) {
			const char *devnode = udev_device_get_devnode(dev);
			if (devnode && strstr(devnode, "/js"))
				hid_device_add(ctx, devnode);
		}
	}

	udev_enumerate_unref(enumerate);
}

struct hid *hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque)
{
	if (!udev_dl_global_init())
		return NULL;

	bool r = true;

	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	ctx->connect = connect;
	ctx->disconnect = disconnect;
	ctx->report = report;
	ctx->opaque = opaque;
	ctx->devices = MTY_HashCreate(0);
	ctx->devices_rev = MTY_HashCreate(0);

	for (uint8_t x = 0; x < HID_FD_MAX; x++) {
		ctx->fds[x].fd = -1;
		ctx->fds[x].events = POLLIN;
	}

	ctx->udev = udev_new();
	if (!ctx->udev) {
		r = false;
		MTY_Log("'udev_new' failed");
		goto except;
	}

	ctx->udev_monitor = udev_monitor_new_from_netlink(ctx->udev, "udev");
	if (!ctx->udev_monitor) {
		r = false;
		MTY_Log("'udev_monitor_new_from_netlink' failed");
		goto except;
	}

	int32_t e = udev_monitor_enable_receiving(ctx->udev_monitor);
	if (e < 0) {
		r = false;
		MTY_Log("'udev_monitor_enable_receiving' failed with error %d", e);
		goto except;
	}

	e = udev_monitor_filter_add_match_subsystem_devtype(ctx->udev_monitor, "input", NULL);
	if (e < 0) {
		r = false;
		MTY_Log("'udev_monitor_filter_add_match_subsystem_devtype' failed with error %d", e);
		goto except;
	}

	ctx->fds[0].fd = udev_monitor_get_fd(ctx->udev_monitor);
	if (ctx->fds[0].fd < 0) {
		r = false;
		MTY_Log("'udev_monitor_get_fd' failed with error %d", ctx->fds[0].fd);
		goto except;
	}

	except:

	if (!r)
		hid_destroy(&ctx);

	return ctx;
}

struct hdevice *hid_get_device_by_id(struct hid *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->devices_rev, id);
}

void hid_poll(struct hid *ctx)
{
	// Fire off an initial enumerate to populate already connected joysticks
	if (!ctx->init_scan) {
		hid_initial_scan(ctx);
		ctx->init_scan = true;
	}

	// Poll the monitor file descriptor (0) and any additional open joysticks
	int32_t e = poll(ctx->fds, HID_FD_MAX, 0);
	if (e <= 0)
		return;

	for (uint8_t x = 0; x < HID_FD_MAX; x++) {
		if (ctx->fds[x].revents & POLLIN) {
			// udev_monitor fd
			if (x == 0) {
				hid_new_device(ctx);

			// joydev event
			} else {
				hid_joystick_event(ctx, ctx->fds[x].fd);
			}
		}
	}
}

void hid_destroy(struct hid **hid)
{
	if (!hid || !*hid)
		return;

	struct hid *ctx = *hid;

	if (ctx->udev_monitor)
		udev_monitor_unref(ctx->udev_monitor);

	if (ctx->udev)
		udev_unref(ctx->udev);

	for (uint8_t x = 0; x < HID_FD_MAX; x++)
		if (ctx->fds[x].fd != -1)
			close(ctx->fds[x].fd);

	MTY_HashDestroy(&ctx->devices, hid_device_destroy);
	MTY_HashDestroy(&ctx->devices_rev, NULL);

	MTY_Free(ctx);
	*hid = NULL;
}

void hid_device_write(struct hdevice *ctx, const void *buf, size_t size)
{
}

bool hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out)
{
	return false;
}

static void hid_gamepad_state(struct hdevice *ctx, MTY_Msg *msg)
{
	msg->type = MTY_MSG_CONTROLLER;

	const MTY_Controller *s = &ctx->state;

	MTY_Controller *c = &msg->controller;
	c->id = s->id;
	c->pid = s->pid;
	c->vid = s->vid;
	c->numButtons = 14;
	c->numValues = 7;

	for (uint8_t x = 0; x < ctx->nbuttons; x++) {
		switch (ctx->btnmap[x]) {
			case BTN_A: c->buttons[MTY_CBUTTON_A] = s->buttons[x]; break;
			case BTN_B: c->buttons[MTY_CBUTTON_B] = s->buttons[x]; break;
			case BTN_X: c->buttons[MTY_CBUTTON_X] = s->buttons[x]; break;
			case BTN_Y: c->buttons[MTY_CBUTTON_Y] = s->buttons[x]; break;
			case BTN_TL: c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = s->buttons[x]; break;
			case BTN_TR: c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = s->buttons[x]; break;
			case BTN_SELECT: c->buttons[MTY_CBUTTON_BACK] = s->buttons[x]; break;
			case BTN_START: c->buttons[MTY_CBUTTON_START] = s->buttons[x]; break;
			case BTN_MODE: c->buttons[MTY_CBUTTON_GUIDE] = s->buttons[x]; break;
			case BTN_THUMBL: c->buttons[MTY_CBUTTON_LEFT_THUMB] = s->buttons[x]; break;
			case BTN_THUMBR: c->buttons[MTY_CBUTTON_RIGHT_THUMB] = s->buttons[x]; break;
		}
	}

	bool up = false;
	bool right = false;
	bool down = false;
	bool left = false;

	for (uint8_t x = 0; x < ctx->naxes; x++) {
		switch (ctx->axmap[x]) {
			case ABS_HAT0X:
				right = s->values[x].data > 0;
				left = s->values[x].data < 0;
				break;
			case ABS_HAT0Y:
				up = s->values[x].data < 0;
				down = s->values[x].data > 0;
				break;
			case ABS_X:
				c->values[MTY_CVALUE_THUMB_LX].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
				c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_LX].min = INT16_MAX;
				break;
			case ABS_Y:
				c->values[MTY_CVALUE_THUMB_LY].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
				c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_LY].min = INT16_MAX;
				break;
			case ABS_Z:
				c->values[MTY_CVALUE_TRIGGER_L].data = s->values[x].data;
				c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
				c->values[MTY_CVALUE_TRIGGER_L].min = INT16_MIN;
				c->values[MTY_CVALUE_TRIGGER_L].min = INT16_MAX;
				break;
			case ABS_RX:
				c->values[MTY_CVALUE_THUMB_RX].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
				c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_RX].min = INT16_MAX;
				break;
			case ABS_RY:
				c->values[MTY_CVALUE_THUMB_RY].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
				c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_RY].min = INT16_MAX;
				break;
			case ABS_RZ:
				c->values[MTY_CVALUE_TRIGGER_R].data = s->values[x].data;
				c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
				c->values[MTY_CVALUE_TRIGGER_R].min = INT16_MIN;
				c->values[MTY_CVALUE_TRIGGER_R].min = INT16_MAX;
				break;

		}
	}

	c->values[MTY_CVALUE_DPAD].data = up && right ? 1 : right && down ? 3 : down && left ? 5 : left && up ? 7 :
		up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
}

static void hid_joystick_state(struct hdevice *ctx, MTY_Msg *msg)
{
	msg->type = MTY_MSG_CONTROLLER;

	const MTY_Controller *s = &ctx->state;

	MTY_Controller *c = &msg->controller;
	c->id = s->id;
	c->pid = s->pid;
	c->vid = s->vid;
	c->numButtons = 14;
	c->numValues = 7;

	for (uint8_t x = 0; x < ctx->nbuttons; x++) {
		switch (ctx->btnmap[x]) {
			case BTN_TRIGGER: c->buttons[MTY_CBUTTON_X] = s->buttons[x]; break;
			case BTN_THUMB: c->buttons[MTY_CBUTTON_A] = s->buttons[x]; break;
			case BTN_THUMB2: c->buttons[MTY_CBUTTON_B] = s->buttons[x]; break;
			case BTN_TOP: c->buttons[MTY_CBUTTON_Y] = s->buttons[x]; break;
			case BTN_TOP2: c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = s->buttons[x]; break;
			case BTN_PINKIE: c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = s->buttons[x]; break;
			case BTN_BASE: c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = s->buttons[x]; break;
			case BTN_BASE2: c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = s->buttons[x]; break;
			case BTN_BASE3: c->buttons[MTY_CBUTTON_BACK] = s->buttons[x]; break;
			case BTN_BASE4: c->buttons[MTY_CBUTTON_START] = s->buttons[x]; break;
			case BTN_BASE5: c->buttons[MTY_CBUTTON_LEFT_THUMB] = s->buttons[x]; break;
			case BTN_BASE6: c->buttons[MTY_CBUTTON_RIGHT_THUMB] = s->buttons[x]; break;
		}
	}

	c->values[MTY_CVALUE_TRIGGER_L].data = s->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].min = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = s->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].min = UINT8_MAX;

	bool up = false;
	bool right = false;
	bool down = false;
	bool left = false;

	for (uint8_t x = 0; x < ctx->naxes; x++) {
		switch (ctx->axmap[x]) {
			case ABS_HAT0X:
				right = s->values[x].data > 0;
				left = s->values[x].data < 0;
				break;
			case ABS_HAT0Y:
				up = s->values[x].data < 0;
				down = s->values[x].data > 0;
				break;
			case ABS_X:
				c->values[MTY_CVALUE_THUMB_LX].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
				c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_LX].min = INT16_MAX;
				break;
			case ABS_Y:
				c->values[MTY_CVALUE_THUMB_LY].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
				c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_LY].min = INT16_MAX;
				break;
			case ABS_Z:
				c->values[MTY_CVALUE_THUMB_RX].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
				c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_RX].min = INT16_MAX;
				break;
			case ABS_RZ:
				c->values[MTY_CVALUE_THUMB_RY].data = s->values[x].data;
				c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
				c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
				c->values[MTY_CVALUE_THUMB_RY].min = INT16_MAX;
				break;

		}
	}

	c->values[MTY_CVALUE_DPAD].data = up && right ? 1 : right && down ? 3 : down && left ? 5 : left && up ? 7 :
		up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
}

void hid_default_state(struct hdevice *ctx, const void *buf, size_t size, MTY_Msg *wmsg)
{
	if (ctx->gamepad) {
		hid_gamepad_state(ctx, wmsg);

	} else {
		hid_joystick_state(ctx, wmsg);
	}
}

void *hid_device_get_state(struct hdevice *ctx)
{
	return NULL;
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

uint32_t hid_device_get_input_report_size(struct hdevice *ctx)
{
	return 0;
}
