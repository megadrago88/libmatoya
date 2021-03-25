// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

#define CRED   "\033[31m"
#define CGREEN "\033[32m"
#define CCLEAR "\033[0m"

#define test_cmp_(name, cmp, val, fmt) { \
	printf("%-20s%-35s%s" fmt "\n", name, #cmp, \
		(cmp) ? CGREEN "Passed" CCLEAR : CRED "Failed" CCLEAR, val); \
	if (!(cmp)) return false; \
}

#define test_cmp(name, cmp) \
	test_cmp_(name, (cmp), "", "%s")

#define test_cmpf(name, cmp, val) \
	test_cmp_(name, (cmp), val, ": %.2f")

#define test_cmpi64(name, cmp, val) \
	test_cmp_(name, (cmp), val, ": %" PRId64)
