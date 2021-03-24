// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#define TEST_FILE MTY_JoinPath(".", "test.file")

static bool file_main(void)
{
	const char *data = "This is arbitrary data.";

	bool r = MTY_WriteFile(TEST_FILE, data, strlen(data));
	test_cmp("MTY_WriteFile", r);

	r = MTY_FileExists(TEST_FILE);
	test_cmp("MTY_FileExists", r);

	size_t size = 0;
	char *rdata = MTY_ReadFile(TEST_FILE, &size);
	test_cmp("MTY_ReadFile", rdata && size == strlen(data));
	test_cmp("MTY_ReadFile", !strcmp(rdata, data));
	MTY_Free(rdata);

	r = MTY_DeleteFile(TEST_FILE);
	test_cmp("MTY_DeleteFile", r);

	r = MTY_FileExists(TEST_FILE);
	test_cmp("MTY_FileExists", !r);

	return true;
}
