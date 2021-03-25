// Memory Test
// Copyright 2021, Jamie Blanks

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions: // The above copyright notice and this
// permission notice shall be included in all copies or substantial portions of the Software. 

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

#include "matoya.h"
#include "test.h"

static bool memory_main (void)
{
	bool failed = false;

	MTY_Free(NULL);
	test_cmp("MTY_Free", !failed);

	MTY_FreeAligned(NULL);
	test_cmp("MTY_FreeAligned", !failed);

	size_t fail_value = 0;
	for (size_t x = 0; x < UINT32_MAX; x += UINT16_MAX) {
		char *buf = (char *) MTY_Alloc(1, x);

		if (buf == NULL) {
			failed = 1;
			fail_value = x;
			break;
		}
		MTY_Free(buf);
	}

	if (failed) {
		test_cmpi64("MTY_Alloc", failed, fail_value);
	} else {
		test_cmp("MTY_Alloc", !failed);
	}

	for (uint8_t x = 0; x < 12; x++) {
		char *buf = (char *) MTY_AllocAligned(0xFFFF, (uint64_t) (1 << x));
		if (buf == NULL || (uintptr_t)buf % (uint64_t) (1 << x)) {
			failed = 1;
			fail_value = (uint64_t)(1 << x);
			break;
		}
		MTY_FreeAligned(buf);
	}

	// Limitation must be power of two
	if (failed) {
		test_cmpi64("MTY_AllocAligned", failed, fail_value);
	} else {
		test_cmp("MTY_AllocAligned", !failed);
	}

	uint8_t *test_buf = (uint8_t *) MTY_Alloc(1,128);
	for (size_t x = 0; x < 128; x++) {
		test_buf[x] = (uint8_t) x;
	}

	test_buf = (uint8_t *) MTY_Realloc(test_buf, 256, 1);
	for (size_t x = 0; x < 128; x++) {
		if (test_buf[x] != x) {
			test_cmpi64("MTY_Realloc1", (test_buf[x] != x), x);
		};
	}
	for (size_t x = 128; x < 256; x++) {
		test_buf[x] = (uint8_t) x;
	}

	test_buf = (uint8_t *) MTY_Realloc(test_buf, 128, 1);
	for (size_t x = 0; x < 128; x++) {
		if (test_buf[x] != x) {
			test_cmpi64("MTY_Realloc2", (test_buf[x] != x), x);
		};
	}
	test_cmp("MTY_Realloc", !failed);

	uint8_t *dup_buf = (uint8_t *) MTY_Dup(test_buf, 128);
	for (size_t x = 0; x < 128; x++) {
		if (dup_buf[x] != test_buf[x]) {
			test_cmpi64("MTY_AllocAligned", failed, x);
		}
	}
	MTY_Free(dup_buf);
	test_cmp("MTY_Dup", !failed);

	for (size_t x = 0; x < 127; x++) {
		test_buf[x] = 'A';
	}
	test_buf[127] = '\0';

	char *dup_str = MTY_Strdup((char *) test_buf);
	for (size_t x = 0; x < 128; x++) {
		if (dup_str[x] != test_buf[x]) {
			test_cmpi64("MTY_Strdup", failed, x);
		}
	}
	MTY_Free(dup_str);
	test_cmp("MTY_Strdup", dup_str[127] == '\0');

	MTY_Strcat(dup_str, 128, (char *) test_buf);
	test_cmp("MTY_Strcat", dup_str[127] == '\0');

	int32_t cmp = MTY_Strcasecmp("abc#&&(!1qwerty", "ABC#&&(!1QWeRTY");
	test_cmp("MTY_Strcasecmp", cmp == 0);

	uint16_t u16_a = 0xBEEF;
	uint32_t u32_a = 0xC0CAC01A;
	uint64_t u64_a = 0xDEADBEEFFACECAFE;

	uint16_t u16_b = MTY_SwapToBE16(u16_a);
	uint32_t u32_b = MTY_SwapToBE32(u32_a);
	uint64_t u64_b = MTY_SwapToBE64(u64_a);

	test_cmp("MTY_SwapToBE16", u16_a != u16_b);
	test_cmp("MTY_SwapToBE32", u32_a != u32_b);
	test_cmp("MTY_SwapToBE64", u32_a != u32_b);

	u16_b = MTY_SwapFromBE16(u16_b);
	u32_b = MTY_SwapFromBE32(u32_b);
	u64_b = MTY_SwapFromBE64(u64_b);

	test_cmp("MTY_SwapFromBE16", u16_a == u16_b);
	test_cmp("MTY_SwapFromBE32", u32_a == u32_b);
	test_cmp("MTY_SwapFromBE64", u32_a == u32_b);

	u16_b = MTY_Swap16(u16_a);
	u32_b = MTY_Swap32(u32_a);
	u64_b = MTY_Swap64(u64_a);

	test_cmp("MTY_Swap16", u16_a != u16_b);
	test_cmp("MTY_Swap32", u32_a != u32_b);
	test_cmp("MTY_Swap64", u32_a != u32_b);


	wchar_t *wide_str = L"11月の雨が私の心に影を落としました。";
	
	char *utf8_str = (char *) MTY_Alloc(2048, sizeof(char));
	wchar_t *wide_str2 = (wchar_t *) MTY_Alloc(2048, sizeof(wchar_t));

	bool succeed = MTY_WideToMulti(wide_str, utf8_str, 2048 * sizeof(char));
	succeed = MTY_MultiToWide(utf8_str, wide_str2, 2048 * sizeof(wchar_t));
	for (size_t x = 0; x < wcslen(wide_str); x++) {
		if (wide_str[x] != wide_str2[x]) {
			test_cmpi64("MTY_MultiToWide", wide_str[x] == wide_str2[x], x);
		}
	}
	test_cmp("MTY_MultiToWide", !failed);
	MTY_Free(utf8_str);
	MTY_Free(wide_str2);

	utf8_str = MTY_WideToMultiD(wide_str);
	wide_str2 = MTY_MultiToWideD(utf8_str);
	for (size_t x = 0; x < wcslen(wide_str); x++) {
		if (wide_str[x] != wide_str2[x]) {
			test_cmpi64("MTY_MultiToWideD", wide_str[x] == wide_str2[x], x);
		}
	}
	test_cmp("MTY_MultiToWideD", !failed);

	MTY_Free(utf8_str);
	MTY_Free(wide_str2);

	MTY_Free(test_buf);

	return !failed;
}

