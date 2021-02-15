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
	if (!fields) fields = "";

	size_t len = sizeof(HTTP_REQUEST_FMT) + strlen(method) + strlen(host) +
		strlen(path) + strlen(fields) + 1;
	char *final = malloc(len);

	snprintf(final, len, HTTP_REQUEST_FMT, method, path, host, fields);

	return final;
}

char *http_connect(const char *host, uint16_t port, const char *fields)
{
	if (!fields) fields = "";

	size_t len = sizeof(HTTP_CONNECT_FMT) + strlen(host) + 32 + strlen(fields) + 1;
	char *final = malloc(len);

	snprintf(final, len, HTTP_CONNECT_FMT, host, port, fields);

	return final;
}

char *http_response(const char *code, const char *msg, const char *fields)
{
	if (!fields) fields = "";

	size_t len = sizeof(HTTP_RESPONSE_FMT) + strlen(code) + strlen(msg) +
		strlen(fields) + 1;
	char *final = malloc(len);

	snprintf(final, len, HTTP_RESPONSE_FMT, code, msg, fields);

	return final;
}

char *http_lc(char *str)
{
	for (int32_t i = 0; str[i]; i++)
		str[i] = (char) tolower(str[i]);

	return str;
}

struct http_header *http_parse_header(char *header)
{
	struct http_header *h = calloc(1, sizeof(struct http_header));

	//http header lines are delimited by "\r\n"
	char *ptr = NULL;
	char *line = MTY_Strtok(header, "\r\n", &ptr);

	for (int8_t first = 1; line; first = 0) {

		//first line is special and is stored seperately
		if (first) {
			h->first_line = MTY_Strdup(line);

		//all lines following the first are in the "key: val" format
		} else {

			char *delim = strpbrk(line, ": ");
			if (delim) {

				//make room for key:val pairs
				h->pairs = realloc(h->pairs, sizeof(struct http_pair) * (h->npairs + 1));

				//place a null character to separate the line
				char save = delim[0];
				delim[0] = '\0';

				//save the key and remove the null character
				h->pairs[h->npairs].key = MTY_Strdup(http_lc(line));
				delim[0] = save;

				//advance the val past whitespace or the : character
				while (*delim && (*delim == ':' || *delim == ' ')) delim++;

				//store the val and increment npairs
				h->pairs[h->npairs].val = MTY_Strdup(delim);
				h->npairs++;
			}
		}

		line = MTY_Strtok(NULL, "\r\n", &ptr);
	}

	return h;
}

void http_free_header(struct http_header *h)
{
	if (!h) return;

	for (uint32_t x = 0; x < h->npairs; x++) {
		free(h->pairs[x].key);
		free(h->pairs[x].val);
	}

	free(h->first_line);
	free(h->pairs);
	free(h);
}

int32_t http_get_header(struct http_header *h, const char *key, int32_t *val_int, char **val_str)
{
	int32_t r = MTY_NET_HTTP_ERR_DEFAULT;

	char *lc_key = MTY_Strdup(key);

	//loop through header pairs and strcmp the key
	for (uint32_t x = 0; x < h->npairs; x++) {
		if (!strcmp(http_lc(lc_key), h->pairs[x].key)) {

			//set val to key str directly if requesting string
			if (val_str) {
				*val_str = h->pairs[x].val;
				r = MTY_NET_HTTP_OK;

			//convert val to int if requesting int
			} else if (val_int) {
				char *endptr = h->pairs[x].val;
				*val_int = strtol(h->pairs[x].val, &endptr, 10);

				if (endptr == h->pairs[x].val) {
					r = MTY_NET_HTTP_ERR_PARSE_HEADER;
				} else r = MTY_NET_HTTP_OK;
			}

			goto except;
		}
	}

	r = MTY_NET_HTTP_ERR_NOT_FOUND;

	except:

	free(lc_key);

	return r;
}

int32_t http_get_status_code(struct http_header *h, uint16_t *status_code)
{
	int32_t r = MTY_NET_HTTP_ERR_DEFAULT;
	char *tok, *ptr = NULL;

	char *tmp_first_line = MTY_Strdup(h->first_line);

	tok = MTY_Strtok(tmp_first_line, " ", &ptr);
	if (!tok) {r = MTY_NET_HTTP_ERR_PARSE_STATUS; goto except;};

	tok = MTY_Strtok(NULL, " ", &ptr);
	if (!tok) {r = MTY_NET_HTTP_ERR_PARSE_STATUS; goto except;};

	*status_code = (uint16_t) strtol(tok, NULL, 10);

	r = MTY_NET_HTTP_OK;

	except:

	free(tmp_first_line);

	return r;
}

char *http_set_header(char *header, const char *name, int32_t type, const void *value)
{
	size_t val_len = (type == HTTP_INT) ? 10 : strlen((char *) value);
	size_t len = header ? strlen(header) : 0;
	size_t new_len = len + strlen(name) + 2 + val_len + 3; //existing len, name len, ": ", val_len, "\r\n\0"

	header = realloc(header, new_len);

	if (type == HTTP_INT) {
		const int32_t *val_int = (const int32_t *) value;
		snprintf(header + len, new_len, "%s: %d\r\n", name, *val_int);

	} else if (type == HTTP_STRING) {
		snprintf(header + len, new_len, "%s: %s\r\n", name, (const char *) value);
	}

	return header;
}

int32_t http_parse_url(const char *url_in, int32_t *scheme, char **host, uint16_t *port, char **path)
{
	if (!url_in || !url_in[0])
		return MTY_NET_HTTP_ERR_PARSE_SCHEME;

	int32_t r = MTY_NET_HTTP_OK;
	char *tok, *ptr = NULL;
	char *tok2, *ptr2 = NULL;

	char *url = MTY_Strdup(url_in);

	*host = NULL;
	*path = NULL;
	*scheme = MTY_SCHEME_NONE;
	*port = 0;

	//scheme
	if (strstr(url, "http") || strstr(url, "ws")) {
		tok = MTY_Strtok(url, ":", &ptr);
		if (!tok) {r = MTY_NET_HTTP_ERR_PARSE_SCHEME; goto except;}

		http_lc(tok);
		if (!strcmp(tok, "https")) {
			*scheme = MTY_SCHEME_HTTPS;
		} else if (!strcmp(tok, "http")) {
			*scheme = MTY_SCHEME_HTTP;
		} else if (!strcmp(tok, "ws")) {
			*scheme = MTY_SCHEME_WS;
		} else if (!strcmp(tok, "wss")) {
			*scheme = MTY_SCHEME_WSS;
		} else {r = MTY_NET_HTTP_ERR_PARSE_SCHEME; goto except;}

		//host + port
		tok = MTY_Strtok(NULL, "/", &ptr);
		if (!tok) {r = MTY_NET_HTTP_ERR_PARSE_HOST; goto except;}

	//no scheme, assume host:port/path syntax
	} else {
		*scheme = MTY_SCHEME_HTTP;
		tok = MTY_Strtok(url, "/", &ptr);
	}

	if (!tok) {r = MTY_NET_HTTP_ERR_PARSE_SCHEME; goto except;}

	//try to find a port
	*host = MTY_Strdup(tok);
	if (!*host) {r = MTY_NET_HTTP_ERR_PARSE_HOST; goto except;}

	tok2 = MTY_Strtok(*host, ":", &ptr2);
	if (tok2)
		tok2 = MTY_Strtok(NULL, ":", &ptr2);

	if (tok2) { //we have a port
		*port = (uint16_t) atoi(tok2);
	} else {
		*port = (*scheme == MTY_SCHEME_HTTPS) ? MTY_NET_PORT_S : MTY_NET_PORT;
	}

	//path
	tok = MTY_Strtok(NULL, "", &ptr);
	if (!tok) tok = "";

	size_t path_len = strlen(tok) + 2;
	*path = malloc(strlen(tok) + 2);
	snprintf(*path, path_len, "/%s", tok);

	except:

	free(url);

	return r;
}
