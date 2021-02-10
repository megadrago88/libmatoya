// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>

#define WS_HEADER_SIZE 14

struct ws_header {
	uint8_t fin;
	uint8_t rsv1;
	uint8_t rsv2;
	uint8_t rsv3;
	uint8_t opcode;
	uint8_t mask;
	uint64_t payload_len;
	uint32_t masking_key;
	int8_t addtl_bytes;
};

char *ws_create_key(void);
char *ws_create_accept_key(const char *key);
int8_t ws_validate_key(const char *key, const char *accept);
void ws_parse_header0(struct ws_header *h, char *buf);
void ws_parse_header1(struct ws_header *h, char *buf);
void ws_mask(char *out, const char *in, uint64_t buf_len, uint32_t mask);
void ws_serialize(struct ws_header *h, const char *payload, char *out, uint64_t *out_size);
