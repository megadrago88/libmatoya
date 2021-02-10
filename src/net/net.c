// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tcp.h"
#include "tls.h"
#include "http.h"
#include "ws.h"

#define LEN_IP4    16
#define LEN_CHUNK  64
#define LEN_ORIGIN 512
#define LEN_HEADER (2 * 1024 * 1024)

struct mty_net_tls_ctx {
	struct tls_state *tlss;
};

struct mty_net_conn {
	char *hout;
	struct http_header *hin;

	struct tcp_context *net;
	struct tls_context *tls;

	void *ctx;
	int32_t (*read)(void *ctx, char *buf, size_t size, int32_t timeout_ms);
	int32_t (*write)(void *ctx, const char *buf, size_t size);

	char *host;
	uint16_t port;

	uint8_t ws_mask;
	char *netbuf;
	uint64_t netbuf_size;
};


// TLS Context

void mty_net_free_tls_ctx(struct mty_net_tls_ctx *uc_tls)
{
	if (!uc_tls) return;

	tlss_free(uc_tls->tlss);

	free(uc_tls);
}

int32_t mty_net_new_tls_ctx(struct mty_net_tls_ctx **uc_tls_in)
{
	struct mty_net_tls_ctx *uc_tls = *uc_tls_in = calloc(1, sizeof(struct mty_net_tls_ctx));

	int32_t e = tlss_alloc(&uc_tls->tlss);
	if (e == MTY_NET_OK) return e;

	mty_net_free_tls_ctx(uc_tls);
	*uc_tls_in = NULL;

	return e;
}

int32_t mty_net_set_cacert(struct mty_net_tls_ctx *uc_tls, const char *cacert, size_t size)
{
	return tlss_load_cacert(uc_tls->tlss, cacert, size);
}

int32_t mty_net_set_cert_and_key(struct mty_net_tls_ctx *uc_tls,
	const char *cert, size_t cert_size, const char *key, size_t key_size)
{
	return tlss_load_cert_and_key(uc_tls->tlss, cert, cert_size, key, key_size);
}


// Connection

struct mty_net_conn *mty_net_new_conn(void)
{
	struct mty_net_conn *ucc = calloc(1, sizeof(struct mty_net_conn));

	return ucc;
}

static void mty_net_attach_net(struct mty_net_conn *ucc)
{
	ucc->ctx = ucc->net;
	ucc->read = tcp_read;
	ucc->write = tcp_write;
}

static void mty_net_attach_tls(struct mty_net_conn *ucc)
{
	ucc->ctx = ucc->tls;
	ucc->read = tls_read;
	ucc->write = tls_write;
}

int32_t mty_net_connect(struct mty_net_tls_ctx *uc_tls, struct mty_net_conn *ucc,
	int32_t scheme, const char *host, uint16_t port, bool verify_host,
	const char *proxy_host, uint16_t proxy_port, int32_t timeout_ms)
{
	//set state
	ucc->host = MTY_Strdup(host);
	ucc->port = port;

	//connect via proxy if specified
	bool use_proxy = proxy_host && proxy_host[0] && proxy_port > 0;
	const char *use_host = use_proxy ? proxy_host : host;
	uint16_t use_port = use_proxy ? proxy_port : port;

	//resolve the hostname into an ip4 address
	char ip4[LEN_IP4];
	int32_t e = tcp_getip4(use_host, ip4, LEN_IP4);
	if (e != MTY_NET_OK) return e;

	//make the net connection
	e = tcp_connect(&ucc->net, ip4, use_port, timeout_ms);
	if (e != MTY_NET_OK) return e;

	//default read/write callbacks
	mty_net_attach_net(ucc);

	//proxy CONNECT request
	if (use_proxy) {
		char *h = http_connect(ucc->host, ucc->port, NULL);

		//write the header to the HTTP client/server
		e = ucc->write(ucc->ctx, h, (uint32_t) strlen(h));
		free(h);
		if (e != MTY_NET_OK) return e;

		//read the response header
		e = mty_net_read_header(ucc, timeout_ms);
		if (e != MTY_NET_OK) return e;

		//get the status code
		int32_t status_code = -1;
		e = mty_net_get_status_code(ucc, &status_code);
		if (e != MTY_NET_OK) return e;
		if (status_code != 200) return MTY_NET_ERR_PROXY;
	}

	if (scheme == MTY_NET_HTTPS || scheme == MTY_NET_WSS) {
		if (!uc_tls) return MTY_NET_TLS_ERR_CONTEXT;

		e = tls_connect(&ucc->tls, uc_tls->tlss, ucc->net, ucc->host, verify_host, timeout_ms);
		if (e != MTY_NET_OK) return e;

		//tls read/write callbacks
		mty_net_attach_tls(ucc);
	}

	return MTY_NET_OK;
}

int32_t mty_net_listen(struct mty_net_conn *ucc, const char *bind_ip4, uint16_t port)
{
	ucc->port = port;

	int32_t e = tcp_listen(&ucc->net, bind_ip4, ucc->port);
	if (e != MTY_NET_OK) return e;

	return MTY_NET_OK;
}

int32_t mty_net_accept(struct mty_net_tls_ctx *uc_tls, struct mty_net_conn *ucc,
	struct mty_net_conn **ucc_new_in, int32_t scheme, int32_t timeout_ms)
{
	struct mty_net_conn *ucc_new = *ucc_new_in = mty_net_new_conn();

	int32_t r = MTY_NET_OK;
	struct tcp_context *new_net = NULL;

	int32_t e = tcp_accept(ucc->net, &new_net, timeout_ms);
	if (e != MTY_NET_OK) {r = e; goto except;}

	ucc_new->net = new_net;
	mty_net_attach_net(ucc_new);

	if (scheme == MTY_NET_HTTPS || scheme == MTY_NET_WSS) {
		if (!uc_tls) return MTY_NET_TLS_ERR_CONTEXT;

		e = tls_accept(&ucc_new->tls, uc_tls->tlss, ucc_new->net, timeout_ms);
		if (e != MTY_NET_OK) {r = e; goto except;}

		//tls read/write callbacks
		mty_net_attach_tls(ucc_new);
	}

	except:

	if (r != MTY_NET_OK) {
		free(ucc_new);
		*ucc_new_in = NULL;
	}

	return r;
}

int32_t mty_net_poll(struct mty_net_conn *ucc, int32_t timeout_ms)
{
	return tcp_poll(ucc->net, TCP_POLLIN, timeout_ms);
}

void mty_net_close(struct mty_net_conn *ucc)
{
	if (!ucc) return;

	tls_close(ucc->tls);
	tcp_close(ucc->net);
	free(ucc->host);
	http_free_header(ucc->hin);
	free(ucc->hout);
	free(ucc->netbuf);
	free(ucc);
}


// Request

void mty_net_set_header_str(struct mty_net_conn *ucc, const char *name, const char *value)
{
	ucc->hout = http_set_header(ucc->hout, name, HTTP_STRING, value);
}

void mty_net_set_header_int(struct mty_net_conn *ucc, const char *name, int32_t value)
{
	ucc->hout = http_set_header(ucc->hout, name, HTTP_INT, &value);
}

void mty_net_free_header(struct mty_net_conn *ucc)
{
	free(ucc->hout);
	ucc->hout = NULL;
}

int32_t mty_net_write_header(struct mty_net_conn *ucc, const char *str0, const char *str1, int32_t type)
{
	//generate the HTTP request/response header
	char *h = (type == MTY_NET_REQUEST) ?
		http_request(str0, ucc->host, str1, ucc->hout) :
		http_response(str0, str1, ucc->hout);

	//write the header to the HTTP client/server
	int32_t e = ucc->write(ucc->ctx, h, (uint32_t) strlen(h));

	free(h);

	return e;
}

int32_t mty_net_write_body(struct mty_net_conn *ucc, const char *body, uint32_t body_len)
{
	return ucc->write(ucc->ctx, body, body_len);
}


// Response

static int32_t mty_net_read_header_(struct mty_net_conn *ucc, char **header, int32_t timeout_ms)
{
	int32_t r = MTY_NET_ERR_MAX_HEADER;

	uint32_t max_header = LEN_HEADER;
	char *h = *header = calloc(max_header, 1);

	for (uint32_t x = 0; x < max_header - 1; x++) {
		int32_t e = ucc->read(ucc->ctx, h + x, 1, timeout_ms);
		if (e != MTY_NET_OK) {r = e; break;}

		if (x > 2 && h[x - 3] == '\r' && h[x - 2] == '\n' && h[x - 1] == '\r' && h[x] == '\n')
			return MTY_NET_OK;
	}

	free(h);
	*header = NULL;

	return r;
}

int32_t mty_net_read_header(struct mty_net_conn *ucc, int32_t timeout_ms)
{
	//free any exiting response headers
	if (ucc->hin) http_free_header(ucc->hin);
	ucc->hin = NULL;

	//read the HTTP response header
	char *header = NULL;
	int32_t e = mty_net_read_header_(ucc, &header, timeout_ms);

	if (e == MTY_NET_OK) {
		//parse the header into the http_header struct
		ucc->hin = http_parse_header(header);
		free(header);
	}

	return e;
}

static int32_t mty_net_read_chunk_len(struct mty_net_conn *ucc, uint32_t *len, int32_t timeout_ms)
{
	int32_t r = MTY_NET_ERR_MAX_CHUNK;

	char chunk_len[LEN_CHUNK];
	memset(chunk_len, 0, LEN_CHUNK);

	for (uint32_t x = 0; x < LEN_CHUNK - 1; x++) {
		int32_t e = ucc->read(ucc->ctx, chunk_len + x, 1, timeout_ms);
		if (e != MTY_NET_OK) {r = e; break;}

		if (x > 0 && chunk_len[x - 1] == '\r' && chunk_len[x] == '\n') {
			chunk_len[x - 1] = '\0';
			*len = strtoul(chunk_len, NULL, 16);
			return MTY_NET_OK;
		}
	}

	*len = 0;

	return r;
}

static int32_t mty_net_response_body_chunked(struct mty_net_conn *ucc, char **body, uint32_t *body_len,
	int32_t timeout_ms, size_t max_body)
{
	uint32_t offset = 0;
	uint32_t chunk_len = 0;

	do {
		//read the chunk size one byte at a time
		int32_t e = mty_net_read_chunk_len(ucc, &chunk_len, timeout_ms);
		if (e != MTY_NET_OK) return e;
		if (offset + chunk_len > max_body) return MTY_NET_ERR_MAX_BODY;

		//make room for chunk and "\r\n" after chunk
		*body = realloc(*body, offset + chunk_len + 2);

		//read chunk into buffer with extra 2 bytes for "\r\n"
		e = ucc->read(ucc->ctx, *body + offset, chunk_len + 2, timeout_ms);
		if (e != MTY_NET_OK) return e;

		offset += chunk_len;

	} while (chunk_len > 0);

	(*body)[offset] = '\0';
	*body_len = offset;

	return MTY_NET_OK;
}

int32_t mty_net_read_body_all(struct mty_net_conn *ucc, char **body, uint32_t *body_len,
	int32_t timeout_ms, size_t max_body)
{
	int32_t r = MTY_NET_OK;

	*body = NULL;
	*body_len = 0;

	//look for chunked response
	if (mty_net_check_header(ucc, "Transfer-Encoding", "chunked")) {
		int32_t e = mty_net_response_body_chunked(ucc, body, body_len, timeout_ms, max_body);
		if (e != MTY_NET_OK) {r = e; goto except;}

	//use Content-Length
	} else {
		int32_t e = mty_net_get_header_int(ucc, "Content-Length", (int32_t *) body_len);
		if (e != MTY_NET_OK) {r = e; goto except;}

		if (*body_len == 0) {r = MTY_NET_ERR_NO_BODY; goto except;}
		if (*body_len > max_body) {r = MTY_NET_ERR_MAX_BODY; goto except;}

		*body = calloc(*body_len + 1, 1);

		e = ucc->read(ucc->ctx, *body, *body_len, timeout_ms);

		if (e != MTY_NET_OK) {r = e; goto except;}
	}

	except:

	if (r != MTY_NET_OK) {
		free(*body);
		*body = NULL;
	}

	return r;
}


// Websockets

int32_t mty_net_ws_connect(struct mty_net_conn *ucc, const char *path, const char *origin,
	int32_t timeout_ms, int32_t *upgrade_status)
{
	int32_t r = MTY_NET_OK;
	*upgrade_status = 0;

	//obligatory websocket headers
	char *sec_key = ws_create_key();
	mty_net_set_header_str(ucc, "Upgrade", "websocket");
	mty_net_set_header_str(ucc, "Connection", "Upgrade");
	mty_net_set_header_str(ucc, "Sec-WebSocket-Key", sec_key);
	mty_net_set_header_str(ucc, "Sec-WebSocket-Version", "13");

	//optional origin header
	if (origin)
		mty_net_set_header_str(ucc, "Origin", origin);

	//write the header
	int32_t e = mty_net_write_header(ucc, "GET", path, MTY_NET_REQUEST);
	if (e != MTY_NET_OK) {r = e; goto except;}

	//we expect a 101 response code from the server
	e = mty_net_read_header(ucc, timeout_ms);
	if (e != MTY_NET_OK) {r = e; goto except;}

	//make sure we have a 101 from the server
	e = mty_net_get_status_code(ucc, upgrade_status);
	if (e != MTY_NET_OK) {r = e; goto except;}
	if (*upgrade_status != 101) {r = MTY_NET_WS_ERR_STATUS; goto except;}

	//validate the security key response
	char *server_sec_key = NULL;
	e = mty_net_get_header_str(ucc, "Sec-WebSocket-Accept", &server_sec_key);
	if (e != MTY_NET_OK) {r = e; goto except;}
	if (!ws_validate_key(sec_key, server_sec_key)) {r = MTY_NET_WS_ERR_KEY; goto except;}

	//client must send masked messages
	ucc->ws_mask = 1;

	except:

	free(sec_key);

	return r;
}

int32_t mty_net_ws_accept(struct mty_net_conn *ucc, const char * const *origins,
	int32_t n_origins, bool secure, int32_t timeout_ms)
{
	//wait for the client's request header
	int32_t e = mty_net_read_header(ucc, timeout_ms);
	if (e != MTY_NET_OK) return e;

	//set obligatory headers
	mty_net_set_header_str(ucc, "Upgrade", "websocket");
	mty_net_set_header_str(ucc, "Connection", "Upgrade");

	//check the origin header against our whitelist
	char *origin = NULL;
	e = mty_net_get_header_str(ucc, "Origin", &origin);
	if (e != MTY_NET_OK) return e;

	//secure origin check
	if (secure) {
		char *scheme = strstr(origin, "https://");
		if (scheme != origin) return MTY_NET_WS_ERR_ORIGIN;
	}

	//the substring MUST came at the end of the origin header, thus a strstr AND a strcmp
	bool origin_ok = false;
	for (int32_t x = 0; x < n_origins; x++) {
		char *match = strstr(origin, origins[x]);
		if (match && !strcmp(match, origins[x])) {origin_ok = true; break;}
	}

	if (!origin_ok) return MTY_NET_WS_ERR_ORIGIN;

	//read the key and set a compliant response header
	char *sec_key = NULL;
	e = mty_net_get_header_str(ucc, "Sec-WebSocket-Key", &sec_key);
	if (e != MTY_NET_OK) return e;

	char *accept_key = ws_create_accept_key(sec_key);
	mty_net_set_header_str(ucc, "Sec-WebSocket-Accept", accept_key);
	free(accept_key);

	//write the response header
	e = mty_net_write_header(ucc, "101", "Switching Protocols", MTY_NET_RESPONSE);
	if (e != MTY_NET_OK) return e;

	//server does not send masked messages
	ucc->ws_mask = 0;

	return MTY_NET_OK;
}

int32_t mty_net_ws_write(struct mty_net_conn *ucc, const char *buf, uint32_t buf_len, uint8_t opcode)
{
	struct ws_header h;
	memset(&h, 0, sizeof(struct ws_header));

	h.fin = 1;
	h.mask = ucc->ws_mask;
	h.opcode = opcode;
	h.payload_len = buf_len;

	//resize the serialized buffer
	if (h.payload_len + WS_HEADER_SIZE > ucc->netbuf_size) {
		free(ucc->netbuf);
		ucc->netbuf_size = h.payload_len + WS_HEADER_SIZE;
		ucc->netbuf = malloc((size_t) ucc->netbuf_size);
	}

	//serialize the payload into a websocket conformant message
	uint64_t out_size = 0;
	ws_serialize(&h, buf, ucc->netbuf, &out_size);

	//write full network buffer
	return ucc->write(ucc->ctx, ucc->netbuf, (uint32_t) out_size);
}

int32_t mty_net_ws_read(struct mty_net_conn *ucc, char *buf, uint32_t buf_len, uint8_t *opcode, int32_t timeout_ms)
{
	char header_buf[WS_HEADER_SIZE];
	struct ws_header h;

	//first two bytes contain most control information
	int32_t e = ucc->read(ucc->ctx, header_buf, 2, timeout_ms);
	if (e != MTY_NET_OK) return e;
	ws_parse_header0(&h, header_buf);
	*opcode = h.opcode;

	//read the payload size and mask
	e = ucc->read(ucc->ctx, header_buf, h.addtl_bytes, timeout_ms);
	if (e != MTY_NET_OK) return e;
	ws_parse_header1(&h, header_buf);

	//check bounds
	if (h.payload_len > INT32_MAX) return MTY_NET_ERR_MAX_BODY;
	if (h.payload_len > buf_len) return MTY_NET_ERR_BUFFER;

	e = ucc->read(ucc->ctx, buf, (uint32_t) h.payload_len, timeout_ms);
	if (e != MTY_NET_OK) return e;

	//unmask the data if necessary
	if (h.mask)
		ws_mask(buf, buf, h.payload_len, h.masking_key);

	return (int32_t) h.payload_len;
}

int32_t mty_net_ws_close(struct mty_net_conn *ucc, uint16_t status_code)
{
	uint16_t status_code_be = MTY_SwapToBE16(status_code);

	return mty_net_ws_write(ucc, (char *) &status_code_be, sizeof(uint16_t), MTY_NET_WSOP_CLOSE);
}


// Helpers

int32_t mty_net_get_status_code(struct mty_net_conn *ucc, int32_t *status_code)
{
	*status_code = 0;
	return http_get_status_code(ucc->hin, status_code);
}

int32_t mty_net_get_header(struct mty_net_conn *ucc, const char *key, int32_t *val_int, char **val_str)
{
	return http_get_header(ucc->hin, key, val_int, val_str);
}

int32_t mty_net_parse_url(char *url, struct mty_net_info *uci)
{
	memset(uci, 0, sizeof(struct mty_net_info));

	return http_parse_url(url, &uci->scheme, &uci->host, &uci->port, &uci->path);
}

int8_t mty_net_check_header(struct mty_net_conn *ucc, const char *name, const char *subval)
{
	char *val = NULL;

	int32_t e = mty_net_get_header_str(ucc, name, &val);

	return (e == MTY_NET_OK && strstr(http_lc(val), subval)) ? 1 : 0;
}

void mty_net_free_info(struct mty_net_info *uci)
{
	free(uci->host);
	free(uci->path);
}
