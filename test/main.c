// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "test_memory.h"

// Framework
#include "test.h"

/// Modules
#include "version.h"
#include "time.h"
#include "test_log.h"
#include "file_test.h"

int32_t main(int32_t argc, char **argv)
{
	if (!version_main())
		return 1;

	if (!time_main())
		return 1;

	if (!file_main())
		return 1;

	if (!memory_main())
    return 1;

	if (!log_main())
		return 1;

	return 0;
}
