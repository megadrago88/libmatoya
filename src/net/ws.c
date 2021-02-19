// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "net.h"
#include "http.h"

enum {
	MTY_NET_WSOP_CONTINUE = 0x0,
	MTY_NET_WSOP_TEXT     = 0x1,
	MTY_NET_WSOP_BINARY   = 0x2,
	MTY_NET_WSOP_CLOSE    = 0x8,
	MTY_NET_WSOP_PING     = 0x9,
	MTY_NET_WSOP_PONG     = 0xA,
};

struct ws_header {
	bool fin;
	bool rsv1;
	bool rsv2;
	bool rsv3;
	bool mask;
	uint8_t opcode;
	size_t payload_len;
	uint32_t masking_key;
	uint8_t addtl_bytes;
};

struct MTY_WebSocket {
	struct mty_net *net;
	bool connected;

	int64_t last_ping;
	int64_t last_pong;
	uint16_t close_code;

	bool mask;
	void *buf;
	size_t size;
};


// Private

#define WS_HEADER_SIZE   14
#define WS_PING_INTERVAL 60000.0f
#define WS_PONG_TO       (WS_PING_INTERVAL * 3.0f)
#define WS_MAGIC          "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define WS_B64_SIZE(s) \
	((((s) + 2) / 3 * 4) + 1)

static char *ws_create_key(void)
{
	uint32_t buf[4];

	MTY_RandomBytes(buf, sizeof(uint32_t) * 4);

	size_t key_len = WS_B64_SIZE(4 * sizeof(uint32_t));
	char *key = MTY_Alloc(key_len, 1);
	MTY_BytesToBase64(buf, 16, key, key_len);

	return key;
}

static char *ws_create_accept_key(const char *key)
{
	size_t buf_len = strlen(key) + strlen(WS_MAGIC) + 1;
	char *buf = MTY_Alloc(buf_len, 1);
	snprintf(buf, buf_len, "%s%s", key, WS_MAGIC);

	uint8_t sha1[MTY_SHA1_SIZE];
	MTY_CryptoHash(MTY_ALGORITHM_SHA1, buf, strlen(buf), NULL, 0, sha1, MTY_SHA1_SIZE);
	MTY_Free(buf);

	size_t akey_len = WS_B64_SIZE(MTY_SHA1_SIZE);
	char *akey = MTY_Alloc(akey_len, 1);
	MTY_BytesToBase64(sha1, MTY_SHA1_SIZE, akey, akey_len);

	return akey;
}

static bool ws_validate_key(const char *key, const char *accept_in)
{
	char *accept_key = ws_create_accept_key(key);
	bool r = !strcmp(accept_key, accept_in);
	MTY_Free(accept_key);

	return r;
}

static void ws_parse_header0(struct ws_header *h, const uint8_t *buf)
{
	uint8_t b = buf[0];
	h->fin = b & 0x80;
	h->rsv1 = b & 0x40;
	h->rsv2 = b & 0x20;
	h->rsv3 = b & 0x10;
	h->opcode = b & 0xF;

	b = buf[1];
	h->mask = b & 0x80;
	h->payload_len = b & 0x7F;

	if (h->payload_len == 126)
		h->addtl_bytes += 2;

	if (h->payload_len == 127)
		h->addtl_bytes += 8;

	if (h->mask)
		h->addtl_bytes += 4;
}

static void ws_parse_header1(struct ws_header *h, const uint8_t *buf)
{
	size_t o = 0;

	// Payload len of < 126 uses 1 bytes, == 126 uses 2 bytes, == 127 uses 8 bytes
	if (h->payload_len == 126) {
		const uint16_t *b16 = (const uint16_t *) buf;
		h->payload_len = MTY_SwapFromBE16(*b16);
		o += 2;

	} else if (h->payload_len == 127) {
		const uint64_t *b64 = (const uint64_t *) buf;
		h->payload_len = (size_t) MTY_SwapFromBE64(*b64);
		o += 8;
	}

	if (h->mask) {
		const uint32_t *b32 = (const uint32_t *) (buf + o);
		h->masking_key = *b32;
	}
}

static void ws_mask(const uint8_t *in, size_t buf_len, uint32_t mask, uint8_t *out)
{
	const uint8_t *key = (const uint8_t *) &mask;

	for (size_t x = 0; x < buf_len; x++)
		out[x] = in[x] ^ key[x % 4];
}

static size_t ws_serialize(const struct ws_header *h, const void *payload, uint8_t *out)
{
	size_t o = 0;
	uint8_t b = 0;

	if (h->fin)
		b |= 0x80;

	if (h->rsv1)
		b |= 0x40;

	if (h->rsv2)
		b |= 0x20;

	if (h->rsv3)
		b |= 0x10;

	b |= h->opcode & 0xF;
	out[o++] = b;

	b = 0;
	if (h->mask)
		b |= 0x80;

	// Payload len calculations -- can use 1, 2, or 8 bytes
	if (h->payload_len < 126) {
		uint8_t l = (uint8_t) h->payload_len;
		b |= l;
		out[o++] = b;

	} else if (h->payload_len >= 126 && h->payload_len <= UINT16_MAX) {
		uint16_t l = MTY_SwapToBE16((uint16_t) h->payload_len);
		b |= 0x7E;
		out[o++] = b;

		memcpy(out + o, &l, 2);
		o += 2;

	} else {
		uint64_t l = MTY_SwapToBE64((uint64_t) h->payload_len);
		b |= 0x7F;
		out[o++] = b;

		memcpy(out + o, &l, 8);
		o += 8;
	}

	// Mask if necessary
	if (h->mask) {
		uint32_t masking_key = 0;
		MTY_RandomBytes(&masking_key, sizeof(uint32_t));
		memcpy(out + o, &masking_key, 4);
		o += 4;

		ws_mask(payload, h->payload_len, masking_key, out + o);

	} else {
		memcpy(out + o, payload, h->payload_len);
	}

	return o + h->payload_len;
}

static bool ws_connect(MTY_WebSocket *ws, const char *path, const char *headers,
	uint32_t timeout, uint16_t *upgrade_status)
{
	char *req = NULL;
	struct http_header *hdr = NULL;

	// Obligatory websocket headers
	char *sec_key = ws_create_key();
	req = http_set_header_str(req, "Upgrade", "websocket");
	req = http_set_header_str(req, "Connection", "Upgrade");
	req = http_set_header_str(req, "Sec-WebSocket-Key", sec_key);
	req = http_set_header_str(req, "Sec-WebSocket-Version", "13");

	// Optional headers
	if (headers)
		req = http_set_all_headers(req, headers);

	// Write the header
	bool r = http_write_request_header(ws->net, "GET", path, req);
	if (!r)
		goto except;

	// We expect a 101 response code from the server
	hdr = http_read_header(ws->net, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	// Make sure we have a 101 from the server
	r = http_get_status_code(hdr, upgrade_status);
	if (!r)
		goto except;

	r = *upgrade_status == 101;
	if (!r)
		goto except;

	// Validate the security key response
	const char *server_sec_key = NULL;
	r = http_get_header_str(hdr, "Sec-WebSocket-Accept", &server_sec_key);
	if (!r)
		goto except;

	r = ws_validate_key(sec_key, server_sec_key);
	if (!r)
		goto except;

	// Client must send masked messages
	ws->mask = true;

	except:

	http_header_destroy(&hdr);
	MTY_Free(sec_key);
	MTY_Free(req);

	return r;
}

static bool ws_accept(MTY_WebSocket *ws, const char * const *origins, uint32_t n_origins,
	bool secure_origin, uint32_t timeout)
{
	bool r = true;
	char *res = NULL;
	struct http_header *hdr = NULL;

	// Wait for the client's request header
	hdr = http_read_header(ws->net, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	// Check the origin header against the whitelist
	const char *origin = NULL;
	r = http_get_header_str(hdr, "Origin", &origin);
	if (!r)
		goto except;

	// Secure origin check
	if (secure_origin) {
		r = strstr(origin, "https://") == origin;
		if (!r)
			goto except;
	}

	// The substring MUST came at the end of the origin header, thus a strstr AND a strcmp
	r = false;
	for (uint32_t x = 0; x < n_origins && !r; x++) {
		const char *match = strstr(origin, origins[x]);
		r = match && !strcmp(match, origins[x]);
	}

	if (!r)
		goto except;

	// Read the key and set a compliant response header
	const char *sec_key = NULL;
	r = http_get_header_str(hdr, "Sec-WebSocket-Key", &sec_key);
	if (!r)
		goto except;

	char *accept_key = ws_create_accept_key(sec_key);
	res = http_set_header_str(res, "Sec-WebSocket-Accept", accept_key);
	MTY_Free(accept_key);

	// Set obligatory headers
	res = http_set_header_str(res, "Upgrade", "websocket");
	res = http_set_header_str(res, "Connection", "Upgrade");

	// Write the response header
	r = http_write_response_header(ws->net, "101", "Switching Protocols", res);
	if (!r)
		goto except;

	// Server does not send masked messages
	ws->mask = false;

	except:

	MTY_Free(res);
	http_header_destroy(&hdr);

	return r;
}

static bool ws_write(MTY_WebSocket *ws, const void *buf, size_t size, uint8_t opcode)
{
	struct ws_header h = {0};
	h.fin = true;
	h.mask = ws->mask;
	h.opcode = opcode;
	h.payload_len = size;

	// Resize the serialized buffer if necessary
	if (h.payload_len + WS_HEADER_SIZE > ws->size) {
		ws->size = h.payload_len + WS_HEADER_SIZE;
		ws->buf = MTY_Realloc(ws->buf, ws->size, 1);
	}

	// Serialize the payload into a websocket conformant message
	size_t out_size = ws_serialize(&h, buf, ws->buf);

	// Write full network buffer
	return mty_net_write(ws->net, ws->buf, out_size);;
}

static bool ws_read(MTY_WebSocket *ws, void *buf, size_t size, uint8_t *opcode, uint32_t timeout, size_t *read)
{
	// First two bytes contain most control information
	uint8_t header_buf[WS_HEADER_SIZE];
	if (!mty_net_read(ws->net, header_buf, 2, timeout))
		return false;

	struct ws_header h = {0};
	ws_parse_header0(&h, header_buf);
	*opcode = h.opcode;

	// Read the payload size and mask
	if (!mty_net_read(ws->net, header_buf, h.addtl_bytes, timeout))
		return false;

	ws_parse_header1(&h, header_buf);

	// Check bounds
	if (h.payload_len > INT32_MAX)
		return false;

	if (h.payload_len > size)
		return false;

	// Read the payload
	if (!mty_net_read(ws->net, buf, h.payload_len, timeout))
		return false;

	// Unmask the data if necessary
	if (h.mask)
		ws_mask(buf, h.payload_len, h.masking_key, buf);

	*read = h.payload_len;

	return true;
}


// Public

MTY_WebSocket *MTY_WebSocketConnect(const char *host, uint16_t port, bool secure, const char *path,
	const char *headers, uint32_t timeout, uint16_t *upgradeStatus)
{
	bool r = true;

	MTY_WebSocket *ctx = MTY_Alloc(1, sizeof(MTY_WebSocket));

	ctx->net = mty_net_connect(host, port, secure, timeout);
	if (!ctx->net) {
		r = false;
		goto except;
	}

	r = ws_connect(ctx, path, headers, timeout, upgradeStatus);
	if (!r)
		goto except;

	ctx->connected = true;
	ctx->last_ping = ctx->last_pong = MTY_Timestamp();

	except:

	if (!r)
		MTY_WebSocketDestroy(&ctx);

	return ctx;
}

MTY_WebSocket *MTY_WebSocketListen(const char *host, uint16_t port)
{
	MTY_WebSocket *ctx = MTY_Alloc(1, sizeof(MTY_WebSocket));

	ctx->net = mty_net_listen(host, port);
	if (!ctx->net)
		MTY_WebSocketDestroy(&ctx);

	return ctx;
}

MTY_WebSocket *MTY_WebSocketAccept(MTY_WebSocket *ws, const char * const *origins, uint32_t numOrigins,
	bool secureOrigin, uint32_t timeout)
{
	MTY_WebSocket *ws_child = NULL;
	struct mty_net *child = mty_net_accept(ws->net, timeout);

	if (child) {
		ws_child = MTY_Alloc(1, sizeof(MTY_WebSocket));
		ws_child->net = child;

		if (ws_accept(ws_child, origins, numOrigins, secureOrigin, timeout)) {
			ws_child->connected = true;
			ws_child->last_ping = ws_child->last_pong = MTY_Timestamp();

		} else {
			MTY_WebSocketDestroy(&ws_child);
		}
	}

	return ws_child;
}

void MTY_WebSocketDestroy(MTY_WebSocket **ws)
{
	if (!ws || !*ws)
		return;

	MTY_WebSocket *ctx = *ws;

	if (ctx->net) {
		if (ctx->connected) {
			uint16_t code_be = MTY_SwapToBE16(1000);
			ws_write(ctx, &code_be, sizeof(uint16_t), MTY_NET_WSOP_CLOSE);
		}

		mty_net_destroy(&ctx->net);
	}

	MTY_Free(ctx->buf);

	MTY_Free(ctx);
	*ws = NULL;
}

MTY_Async MTY_WebSocketRead(MTY_WebSocket *ws, char *msg, size_t size, uint32_t timeout)
{
	// Implicit ping handler
	int64_t now = MTY_Timestamp();
	if (MTY_TimeDiff(ws->last_ping, now) > WS_PING_INTERVAL) {
		if (!ws_write(ws, "ping", 4, MTY_NET_WSOP_PING))
			return MTY_ASYNC_ERROR;

		ws->last_ping = now;
	}

	// Poll for more data
	MTY_Async r = mty_net_poll(ws->net, timeout);

	if (r == MTY_ASYNC_OK) {
		uint8_t opcode = MTY_NET_WSOP_CONTINUE;
		memset(msg, 0, size);

		size_t n = 0;
		if (ws_read(ws, msg, size - 1, &opcode, 1000, &n)) {
			switch (opcode) {
				case MTY_NET_WSOP_PING:
					r = ws_write(ws, msg, n, MTY_NET_WSOP_PONG) ? MTY_ASYNC_CONTINUE : MTY_ASYNC_ERROR;
					break;

				case MTY_NET_WSOP_PONG:
					ws->last_pong = MTY_Timestamp();
					r = MTY_ASYNC_CONTINUE;
					break;

				case MTY_NET_WSOP_TEXT:
					if (n == 0)
						r = MTY_ASYNC_CONTINUE;
					break;

				case MTY_NET_WSOP_CLOSE: {
					memcpy(&ws->close_code, msg, 2);
					ws->close_code = MTY_SwapFromBE16(ws->close_code);

					r = MTY_ASYNC_DONE;
					break;
				}

				default:
					r = MTY_ASYNC_CONTINUE;
					break;
			}
		} else {
			r = MTY_ASYNC_ERROR;
		}
	}

	// If we haven't gotten a pong within WS_PONG_TO, error
	if (MTY_TimeDiff(ws->last_pong, MTY_Timestamp()) > WS_PONG_TO)
		r = MTY_ASYNC_ERROR;

	return r;
}

bool MTY_WebSocketWrite(MTY_WebSocket *ws, const char *msg)
{
	return ws_write(ws, msg, strlen(msg), MTY_NET_WSOP_TEXT);
}

uint16_t MTY_WebSocketGetCloseCode(MTY_WebSocket *ctx)
{
	return ctx->close_code;
}
