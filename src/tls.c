// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

bool MTY_IsTLSHandshake(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 &&
		((d[0] == 0x14 || d[0] == 0x16) &&  // Change Cipher Spec, Handshake
		((d[1] == 0xFE && d[2] == 0xFD) ||  // DTLS 1.2
		(d[1] == 0x03 && d[2] == 0x03)));   // TLS 1.2
}

bool MTY_IsTLSApplicationData(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 &&
		d[0] == 0x17 &&                    // Application Data
		((d[1] == 0xFE && d[2] == 0xFD) || // DTLS 1.2
		(d[1] == 0x03 && d[2] == 0x03));   // TLS 1.2
}
