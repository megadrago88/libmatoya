// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <stdio.h>

#include "tlocal.h"

const char *MTY_VersionString(uint32_t platform)
{
	uint8_t major = (platform & 0xFF00) >> 8;
	uint8_t minor = platform & 0xFF;

	char *ver = mty_tlocal(16);

	if (minor > 0) {
		snprintf(ver, 16, "%u.%u", major, minor);

	} else {
		snprintf(ver, 16, "%u", major);
	}

	return ver;
}
