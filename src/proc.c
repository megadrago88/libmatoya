// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include "execv.h"

bool MTY_RestartProcess(int32_t argc, char * const *argv)
{
	if (argc > 0) {
		char **argvn = MTY_Alloc(argc + 1, sizeof(char *));
		for (int32_t x = 0; x < argc; x++)
			argvn[x] = argv[x];

		bool r = mty_execv(MTY_ProcessName(), argvn);
		MTY_Free(argvn);

		return r;
	}

	return mty_execv(MTY_ProcessName(), argv);
}
