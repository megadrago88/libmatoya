#pragma once

#include <stdint.h>
#include <stdbool.h>

int32_t gzip_compress(char *in, int32_t in_size, char **out, int32_t *out_size);
int32_t gzip_decompress(char *in, int32_t in_size, char **out, int32_t *out_size);
