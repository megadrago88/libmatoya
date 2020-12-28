// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdbool.h>
#include <process.h>

static bool mty_execv(const char *name, char * const *argv)
{
	wchar_t *namew = MTY_MultiToWideD(name);

	// We assume a maximum of 127 args and a NULL arg
	wchar_t *argvw[128] = {0};

	for (uint8_t x = 0; x < 127 && argv[x]; x++)
		argvw[x] = MTY_MultiToWideD(argv[x]);

	_wexecv(namew, argvw);
	MTY_Log("'_wexecv' failed with errno %d", errno);

	for (uint8_t x = 0; x < 127; x++)
		MTY_Free(argvw[x]);

	MTY_Free(namew);

	return false;
}
