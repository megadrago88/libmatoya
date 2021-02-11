// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "gzip.h"

#include <string.h>
#include <stdlib.h>

#include "miniz/miniz.c"

#define MTY_GZIP_CHUNK_SIZE (256 * 1024) // 256KB

void *mty_gzip_decompress(const void *in, size_t inSize, size_t *outSize)
{
	void *out = NULL;
	*outSize = 0;

	z_stream strm = {0};
	int32_t e = inflateInit2(&strm, 16 + MAX_WBITS);
	if (e != Z_OK) {
		MTY_Log("'inflateInit2' failed with error %d", e);
		return NULL;
	}

	strm.avail_in = (uint32_t) inSize;
	strm.next_in = in;

	do {
		out = MTY_Realloc(out, *outSize + MTY_GZIP_CHUNK_SIZE, 1);

		strm.avail_out = MTY_GZIP_CHUNK_SIZE;
		strm.next_out = (Bytef *) out + *outSize;

		e = inflate(&strm, Z_NO_FLUSH);

		*outSize += MTY_GZIP_CHUNK_SIZE - strm.avail_out;

	} while (e == Z_OK);

	if (e != Z_STREAM_END) {
		MTY_Log("'inflate' failed with error %d", e);

		MTY_Free(out);
		*outSize = 0;

		return NULL;
	}

	inflateEnd(&strm);

	return out;
}
