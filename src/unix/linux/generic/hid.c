// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "hid/hid.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/joystick.h>

#include "udev-dl.h"

// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/joystick.h

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

struct hat {
	bool up;
	bool right;
	bool down;
	bool left;
};

struct hdevice {
	MTY_Controller state;
	bool gamepad; // PS4, XInput
	uint16_t btnmap[HID_BTNMAP_MAX];
	uint8_t axmap[HID_AXMAP_MAX];
	struct hat hat;
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

static void hid_device_vid_pid(const char *syspath, uint16_t *vid, uint16_t *pid)
{
	char *dsyspath = MTY_Strdup(syspath);

	char *end = strstr(dsyspath, "/input");
	if (end) {
		*end = '\0';

		char *begin = strrchr(dsyspath, '/');
		if (begin) {
			begin += 1;

			end = strchr(begin, '.');
			if (end) {
				*end = '\0';

				begin = strchr(begin, ':');
				if (begin) {
					begin += 1;

					char *split = strchr(begin, ':');
					*split = '\0';

					*vid = strtol(begin, NULL, 16);
					*pid = strtol(split + 1, NULL, 16);
				}
			}
		}
	}

	MTY_Free(dsyspath);
}

static void hid_device_add(struct hid *ctx, const char *devnode, const char *syspath)
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
		hdev->state.numValues = 1; // There's always a DPAD
		hdev->state.id = hdev->id;

		hid_device_vid_pid(syspath, &hdev->state.vid, &hdev->state.pid);

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
	const char *syspath = udev_device_get_syspath(dev);
	if (!action || !devnode)
		goto except;

	if (!strstr(devnode, "/js"))
		goto except;

	if (!strcmp(action, "add")) {
		hid_device_add(ctx, devnode, syspath);

	} else if (!strcmp(action, "remove")) {
		hid_device_remove(ctx, devnode);
	}

	except:

	udev_device_unref(dev);
}

static void hid_set_hat(struct hdevice *ctx, uint8_t type, const struct js_event *event)
{
	struct hat *h = &ctx->hat;

	if (type == ABS_HAT0X || type == ABS_HAT0Y) {
		if (type == ABS_HAT0X) {
			h->right = event->value > 0;
			h->left = event->value < 0;

		} else if (type == ABS_HAT0Y) {
			h->up = event->value < 0;
			h->down = event->value > 0;
		}
	}

	ctx->state.values[0].data = h->up && h->right ? 1 : h->right && h->down ? 3 : h->down && h->left ? 5 :
		h->left && h->up ? 7 : h->up ? 0 : h->right ? 2 : h->down ? 4 : h->left ? 6 : 8;
	ctx->state.values[0].usage = 0x39;
	ctx->state.values[0].min = 0;
	ctx->state.values[0].max = 7;
}

static MTY_CButton hid_button(uint16_t type)
{
	switch (type) {
		// Gamepad
		case BTN_A: return MTY_CBUTTON_A;
		case BTN_B: return MTY_CBUTTON_B;
		case BTN_X: return MTY_CBUTTON_X;
		case BTN_Y: return MTY_CBUTTON_Y;
		case BTN_TL: return MTY_CBUTTON_LEFT_SHOULDER;
		case BTN_TR: return MTY_CBUTTON_RIGHT_SHOULDER;
		case BTN_TL2: return MTY_CBUTTON_LEFT_TRIGGER;
		case BTN_TR2: return MTY_CBUTTON_RIGHT_TRIGGER;
		case BTN_SELECT: return MTY_CBUTTON_BACK;
		case BTN_START: return MTY_CBUTTON_START;
		case BTN_MODE: return MTY_CBUTTON_GUIDE;
		case BTN_THUMBL: return MTY_CBUTTON_LEFT_THUMB;
		case BTN_THUMBR: return MTY_CBUTTON_RIGHT_THUMB;

		// Gamepad Unknown
		case BTN_C: return MTY_CBUTTON_TOUCHPAD;
		case BTN_Z: return MTY_CBUTTON_TOUCHPAD + 1;

		// Joystick
		case BTN_TRIGGER: return MTY_CBUTTON_X;
		case BTN_THUMB: return MTY_CBUTTON_A;
		case BTN_THUMB2: return MTY_CBUTTON_B;
		case BTN_TOP: return MTY_CBUTTON_Y;
		case BTN_TOP2: return MTY_CBUTTON_LEFT_SHOULDER;
		case BTN_PINKIE: return MTY_CBUTTON_RIGHT_SHOULDER;
		case BTN_BASE: return MTY_CBUTTON_LEFT_TRIGGER;
		case BTN_BASE2: return MTY_CBUTTON_RIGHT_TRIGGER;
		case BTN_BASE3: return MTY_CBUTTON_BACK;
		case BTN_BASE4: return MTY_CBUTTON_START;
		case BTN_BASE5: return MTY_CBUTTON_LEFT_THUMB;
		case BTN_BASE6: return MTY_CBUTTON_RIGHT_THUMB;

		// Joystick Unknown
		case BTN_DEAD: return MTY_CBUTTON_GUIDE;
	}

	return -1;
}

static uint16_t hid_gamepad_usage(struct hdevice *ctx, uint16_t type, const struct js_event *event)
{
	switch (type) {
		case ABS_X: return 0x30;
		case ABS_Y: return 0x31;
		case ABS_Z: return 0x33;
		case ABS_RX: return 0x32;
		case ABS_RY: return 0x35;
		case ABS_RZ: return 0x34;
	}

	return 0;
}

static uint16_t hid_joystick_usage(struct hdevice *ctx, uint16_t type, const struct js_event *event)
{
	switch (type) {
		case ABS_X: return 0x30;
		case ABS_Y: return 0x31;
		case ABS_Z: return 0x32;
		case ABS_RZ: return 0x35;
	}

	return 0;
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
		MTY_CButton cb = hid_button(hdev->btnmap[event.number]);

		if (cb >= 0) {
			if (cb >= MTY_CBUTTON_MAX)
				return;

			if (cb >= c->numButtons)
				c->numButtons = cb + 1;

			c->buttons[cb] = event.value ? true : false;
		}

	} else if ((event.type & 0xF) == JS_EVENT_AXIS) {
		uint16_t type = hdev->axmap[event.number];
		hid_set_hat(hdev, type, &event);

		uint16_t usage = hdev->gamepad ? hid_gamepad_usage(hdev, type, &event) :
			hid_joystick_usage(hdev, type, &event);

		if (usage > 0) {
			if (event.number + 1 >= MTY_CVALUE_MAX)
				return;

			if (event.number + 1 >= c->numValues)
				c->numValues = event.number + 2;

			c->values[event.number + 1].data  = event.value;
			c->values[event.number + 1].usage  = usage;
			c->values[event.number + 1].min = -INT16_MAX;
			c->values[event.number + 1].max = INT16_MAX;
		}
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
				hid_device_add(ctx, devnode, name);
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

void hid_default_state(struct hdevice *ctx, const void *buf, size_t size, MTY_Msg *wmsg)
{
	wmsg->type = MTY_MSG_CONTROLLER;
	wmsg->controller = ctx->state;
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

bool hid_device_force_default(struct hdevice *ctx)
{
	return true;
}
