// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "matoya.h"

#define HID_STATE_MAX 1024

struct hid;
struct hdevice;

typedef void (*HID_CONNECT)(struct hdevice *device, void *opaque);
typedef void (*HID_DISCONNECT)(struct hdevice *device, void *opaque);
typedef void (*HID_REPORT)(struct hdevice *device, const void *buf, size_t size, void *opaque);

struct hid *hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque);
struct hdevice *hid_get_device_by_id(struct hid *ctx, uint32_t id);
void hid_poll(struct hid *hid);
void hid_destroy(struct hid **hid);

void hid_device_write(struct hdevice *ctx, const void *buf, size_t size);
bool hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out);
void *hid_device_get_state(struct hdevice *ctx);
uint16_t hid_device_get_vid(struct hdevice *ctx);
uint16_t hid_device_get_pid(struct hdevice *ctx);
uint32_t hid_device_get_id(struct hdevice *ctx);
uint32_t hid_device_get_input_report_size(struct hdevice *ctx);
bool hid_device_force_default(struct hdevice *ctx);

void hid_default_state(struct hdevice *ctx, const void *buf, size_t size, MTY_Msg *wmsg);
void hid_default_rumble(struct hid *ctx, uint32_t id, uint16_t low, uint16_t high);

// Win32 specific for interop with RAWINPUT
void hid_win32_listen(void *hwnd);
void hid_win32_device_change(struct hid *ctx, intptr_t wparam, intptr_t lparam);
void hid_win32_report(struct hid *ctx, intptr_t device, const void *buf, size_t size);

// Win32 XInput specifc (XInput devices are ignored by callbacks)
void hid_xinput_rumble(struct hid *ctx, uint32_t id, uint16_t low, uint16_t high);
void hid_xinput_state(struct hid *hid, MTY_MsgFunc func, void *opaque);
