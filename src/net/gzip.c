#include "matoya.h"
#include "gzip.h"

#include <string.h>
#include <stdlib.h>

#include "miniz/miniz.c"

#define GZIP_CHUNK_SIZE (256 * 1024) //256KB

int32_t gzip_compress(char *in, int32_t in_size, char **out, int32_t *out_size)
{
	*out = NULL;
	*out_size = 0;

	z_stream strm = {0};
	int32_t e = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
		16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
	if (e != Z_OK) return MTY_NET_ERR_DEFLATE;

	strm.avail_in = in_size;
	strm.next_in = (Bytef *) in;

	int32_t flush = Z_NO_FLUSH;

	do {
		*out = realloc(*out, *out_size + GZIP_CHUNK_SIZE);

		strm.avail_out = GZIP_CHUNK_SIZE;
		strm.next_out = (Bytef *) *out + *out_size;

		e = deflate(&strm, flush);

		*out_size += GZIP_CHUNK_SIZE - strm.avail_out;

		if (strm.avail_in == 0) flush = Z_FINISH;

	} while (e == Z_OK);

	if (e != Z_STREAM_END) return MTY_NET_ERR_DEFLATE;

	deflateEnd(&strm);

	return MTY_NET_OK;
}

int32_t gzip_decompress(char *in, int32_t in_size, char **out, int32_t *out_size)
{
	*out = NULL;
	*out_size = 0;

	z_stream strm = {0};
	int32_t e = inflateInit2(&strm, 16 + MAX_WBITS);
	if (e != Z_OK) return MTY_NET_ERR_INFLATE;

	strm.avail_in = in_size;
	strm.next_in = (Bytef *) in;

	do {
		*out = realloc(*out, *out_size + GZIP_CHUNK_SIZE);

		strm.avail_out = GZIP_CHUNK_SIZE;
		strm.next_out = (Bytef *) *out + *out_size;

		e = inflate(&strm, Z_NO_FLUSH);

		*out_size += GZIP_CHUNK_SIZE - strm.avail_out;

	} while (e == Z_OK);

	if (e != Z_STREAM_END)  return MTY_NET_ERR_INFLATE;

	inflateEnd(&strm);

	return MTY_NET_OK;
}
