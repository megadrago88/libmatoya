// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "http.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct http_pair {
	char *key;
	char *val;
};

struct http_header {
	char *first_line;
	struct http_pair *pairs;
	uint32_t npairs;
};


// Standard header generators

static const char HTTP_REQUEST_FMT[] =
	"%s %s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"%s"
	"\r\n";

static const char HTTP_CONNECT_FMT[] =
	"CONNECT %s:%u HTTP/1.1\r\n"
	"%s"
	"\r\n";

static const char HTTP_RESPONSE_FMT[] =
	"HTTP/1.1 %s %s\r\n"
	"%s"
	"\r\n";

char *http_request(const char *method, const char *host, const char *path, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_REQUEST_FMT, method, path, host, fields);
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_REQUEST_FMT, method, path, host, fields);

	return str;
}

char *http_connect(const char *host, uint16_t port, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_CONNECT_FMT, host, port, fields);
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_CONNECT_FMT, host, port, fields);

	return str;
}

char *http_response(const char *code, const char *msg, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_RESPONSE_FMT, code, msg, fields);
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_RESPONSE_FMT, code, msg, fields);

	return str;
}


// Header parsing, construction

struct http_header *http_parse_header(const char *header)
{
	struct http_header *h = MTY_Alloc(1, sizeof(struct http_header));
	char *dup = MTY_Strdup(header);

	// HTTP header lines are delimited by "\r\n"
	char *ptr = NULL;
	char *line = MTY_Strtok(dup, "\r\n", &ptr);

	for (bool first = true; line; first = false) {

		// First line is special and is stored seperately
		if (first) {
			h->first_line = MTY_Strdup(line);

		// All lines following the first are in the "key: val" format
		} else {
			char *delim = strpbrk(line, ": ");

			if (delim) {
				h->pairs = MTY_Realloc(h->pairs, h->npairs + 1, sizeof(struct http_pair));

				// Place a null character to separate the line
				char save = delim[0];
				delim[0] = '\0';

				// Save the key and remove the null character
				h->pairs[h->npairs].key = MTY_Strdup(line);
				delim[0] = save;

				// Advance the val past whitespace or the : character
				while (*delim && (*delim == ':' || *delim == ' '))
					delim++;

				// Store the val and increment npairs
				h->pairs[h->npairs].val = MTY_Strdup(delim);
				h->npairs++;
			}
		}

		line = MTY_Strtok(NULL, "\r\n", &ptr);
	}

	MTY_Free(dup);

	return h;
}

void http_free_header(struct http_header **header)
{
	if (!header || !*header)
		return;

	struct http_header *h = *header;

	for (uint32_t x = 0; x < h->npairs; x++) {
		MTY_Free(h->pairs[x].key);
		MTY_Free(h->pairs[x].val);
	}

	MTY_Free(h->first_line);
	MTY_Free(h->pairs);

	MTY_Free(h);
	*header = NULL;
}

bool http_get_status_code(struct http_header *h, uint16_t *status_code)
{
	bool r = true;
	char *tmp = MTY_Strdup(h->first_line);

	char *ptr = NULL;
	char *tok = MTY_Strtok(tmp, " ", &ptr);
	if (!tok) {
		r = false;
		goto except;
	}

	tok = MTY_Strtok(NULL, " ", &ptr);
	if (!tok) {
		r = false;
		goto except;
	}

	*status_code = (uint16_t) strtol(tok, NULL, 10);

	except:

	MTY_Free(tmp);

	return r;
}

bool http_get_header_int(struct http_header *h, const char *key, int32_t *val)
{
	for (uint32_t x = 0; x < h->npairs; x++) {
		if (!MTY_Strcasecmp(key, h->pairs[x].key)) {
			char *endptr = h->pairs[x].val;
			*val = strtol(h->pairs[x].val, &endptr, 10);

			return endptr != h->pairs[x].val;
		}
	}

	return false;
}

bool http_get_header_str(struct http_header *h, const char *key, const char **val)
{
	for (uint32_t x = 0; x < h->npairs; x++) {
		if (!MTY_Strcasecmp(key, h->pairs[x].key)) {
			*val = h->pairs[x].val;
			return true;
		}
	}

	return false;
}

char *http_set_header(char *header, const char *name, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	size_t size = vsnprintf(NULL, 0, fmt, args);
	char *val = MTY_Alloc(size, 1);

	vsnprintf(val, size, fmt, args);
	va_end(args);

	size_t fsize = snprintf(NULL, 0, "%s: %s\r\n", name, val);
	char *fval = MTY_Alloc(fsize, 1);

	size_t total = strlen(header) + fsize;
	header = MTY_Realloc(header, total, 1);
	MTY_Strcat(header, total, fval);

	MTY_Free(fval);
	MTY_Free(val);

	return header;
}


// Misc

bool http_parse_url(const char *url, bool *secure, char **host, uint16_t *port, char **path)
{
	if (!url || !url[0])
		return false;

	bool r = true;
	char *dup = MTY_Strdup(url);

	*host = NULL;
	*path = NULL;
	*secure = false;
	*port = 0;

	// Scheme
	char *tok, *ptr = NULL;

	if (strstr(dup, "http") || strstr(dup, "ws")) {
		tok = MTY_Strtok(dup, ":", &ptr);
		if (!tok) {
			r = false;
			goto except;
		}

		if (!MTY_Strcasecmp(tok, "https") || !MTY_Strcasecmp(tok, "wss")) {
			*secure = true;

		} else if (!MTY_Strcasecmp(tok, "http") || !MTY_Strcasecmp(tok, "ws")) {
			*secure = false;

		} else {
			r = false;
			goto except;
		}

		// Host + port
		tok = MTY_Strtok(NULL, "/", &ptr);
		if (!tok) {
			r = false;
			goto except;
		}

	// No scheme, assume host:port/path syntax
	} else {
		*secure = false;
		tok = MTY_Strtok(dup, "/", &ptr);
	}

	if (!tok) {
		r = false;
		goto except;
	}

	// Try to find a port
	*host = MTY_Strdup(tok);
	if (!*host) {
		r = false;
		goto except;
	}

	char *tok2, *ptr2 = NULL;
	tok2 = MTY_Strtok(*host, ":", &ptr2);
	if (tok2)
		tok2 = MTY_Strtok(NULL, ":", &ptr2);

	if (tok2) { // We have a port
		*port = (uint16_t) atoi(tok2);
	} else {
		*port = *secure ? MTY_NET_PORT_S : MTY_NET_PORT;
	}

	// Path
	tok = MTY_Strtok(NULL, "", &ptr);
	if (!tok) tok = "";

	size_t path_len = strlen(tok) + 2;
	*path = MTY_Alloc(strlen(tok) + 2, 1);
	snprintf(*path, path_len, "/%s", tok);

	except:

	MTY_Free(dup);

	return r;
}
