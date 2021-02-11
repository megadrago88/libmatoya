// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

int32_t gzip_compress(char *in, int32_t in_size, char **out, int32_t *out_size);
int32_t gzip_decompress(char *in, int32_t in_size, char **out, int32_t *out_size);
