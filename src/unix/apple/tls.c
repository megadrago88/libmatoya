// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <Security/SecureTransport.h>
#include <Security/Security.h>

#include "cert.h"

struct tls_write {
	void *out;
	size_t size;
	size_t *written;
};

struct MTY_Cert {
	SecKeyRef public_key;
	SecKeyRef private_key;
	SecCertificateRef cert;
	SecIdentityRef ident;
};

struct MTY_TLS {
	char *fp;

	uint8_t *buf;
	size_t size;
	size_t offset;

	void *opaque;
	struct tls_write *w;
	MTY_TLSWriteFunc write_func;

	SSLContextRef ctx;
};

#define TLS_MTU_PADDING 64


// Cert

MTY_Cert *MTY_CertCreate(void)
{
	bool r = true;
	CFDataRef export = NULL;
	CFMutableDictionaryRef attrs = NULL;

	MTY_Cert *ctx = MTY_Alloc(1, sizeof(MTY_Cert));

	uint8_t *buf = MTY_Alloc(sizeof(CERT_TEMPLATE) + CERT_SIG_LENGTH, 1);
	memcpy(buf, CERT_TEMPLATE, sizeof(CERT_TEMPLATE));

	// Private key
	attrs = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
	CFDictionarySetValue(attrs, kSecAttrKeyType, kSecAttrKeyTypeRSA);

	int32_t bits = 2048;
	CFNumberRef cfbits = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &bits);
	CFDictionarySetValue(attrs, kSecAttrKeySizeInBits, cfbits);
	CFRelease(cfbits);

	OSStatus e = SecKeyGeneratePair(attrs, &ctx->public_key, &ctx->private_key);
	if (e != errSecSuccess) {
		MTY_Log("'SecKeyGeneratePair' failed with error %d", e);
		r = false;
		goto except;
	}

	e = SecRandomCopyBytes(kSecRandomDefault, CERT_SERIAL_LENGTH, buf + CERT_SERIAL_OFFSET);
	if (e != errSecSuccess) {
		MTY_Log("'SecRandomCopyBytes' failed with error %d", e);
		r = false;
		goto except;
	}

	buf[CERT_SERIAL_OFFSET] &= 0x7F; // Non-negative

	// Dates
	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
	CFDateRef date_issue = CFDateCreate(NULL, now - (60 * 60));
	CFDateRef date_exp = CFDateCreate(NULL, now + (60 * 60 * 24 * 30));

	CFDateFormatterRef formatter = CFDateFormatterCreate(NULL, NULL, kCFDateFormatterNoStyle, kCFDateFormatterNoStyle);
	CFDateFormatterSetFormat(formatter, CFSTR("yyMMddHHmmss'Z'"));

	CFStringRef str_issue = CFDateFormatterCreateStringWithDate(NULL, formatter, date_issue);
	CFStringRef str_exp = CFDateFormatterCreateStringWithDate(NULL, formatter, date_exp);

	char cstr_issue[CERT_DATE_LENGTH + 8] = {0};
	char cstr_exp[CERT_DATE_LENGTH + 8] = {0};

	CFStringGetCString(str_issue, cstr_issue, CERT_DATE_LENGTH + 8, kCFStringEncodingUTF8);
	CFStringGetCString(str_exp, cstr_exp, CERT_DATE_LENGTH + 8, kCFStringEncodingUTF8);

	memcpy(buf + CERT_ISSUE_DATE_OFFSET, cstr_issue, CERT_DATE_LENGTH);
	memcpy(buf + CERT_EXP_DATE_OFFSET, cstr_exp, CERT_DATE_LENGTH);

	CFRelease(str_exp);
	CFRelease(str_issue);
	CFRelease(formatter);
	CFRelease(date_exp);
	CFRelease(date_issue);

	// Copy public Key
	e = SecItemExport(ctx->public_key, kSecFormatBSAFE, 0, NULL, &export);
	if (e != errSecSuccess) {
		MTY_Log("'SecItemExport' failed with error %d", e);
		r = false;
		goto except;
	}

	if (CFDataGetLength(export) != CERT_PUBLIC_KEY_LENGTH) {
		MTY_Log("Exported public key is the wrong size");
		r = false;
		goto except;
	}

	memcpy(buf + CERT_PUBLIC_KEY_OFFSET, CFDataGetBytePtr(export), CERT_PUBLIC_KEY_LENGTH);

	// Signing
	CFDataRef csr = CFDataCreate(NULL, buf + CERT_CSR_OFFSET, CERT_CSR_LENGTH);
	SecTransformRef transform = SecSignTransformCreate(ctx->private_key, NULL);
	SecTransformSetAttribute(transform, kSecDigestTypeAttribute, kSecDigestSHA1, NULL);
	SecTransformSetAttribute(transform, kSecTransformInputAttributeName, csr, NULL);

	CFDataRef sig = SecTransformExecute(transform, NULL);
	memcpy(buf + sizeof(CERT_TEMPLATE), CFDataGetBytePtr(sig), CERT_SIG_LENGTH);

	CFRelease(sig);
	CFRelease(csr);
	CFRelease(transform);

	// Create the cert
	CFDataRef cfcert = CFDataCreate(NULL, buf, sizeof(CERT_TEMPLATE) + CERT_SIG_LENGTH);
	ctx->cert = SecCertificateCreateWithData(NULL, cfcert);
	CFRelease(cfcert);

	// Create the identity
	SecIdentityCreateWithCertificate(NULL, ctx->cert, &ctx->ident);

	except:

	if (export)
		CFRelease(export);

	if (attrs)
		CFRelease(attrs);

	MTY_Free(buf);

	if (!r)
		MTY_CertDestroy(&ctx);

	return ctx;
}

static void tls_cert_to_fingerprint(SecCertificateRef cert, char *fingerprint, size_t size)
{
	snprintf(fingerprint, size, "sha-256 ");

	uint8_t buf[MTY_SHA256_SIZE];

	CFDataRef bytes = SecCertificateCopyData(cert);

	MTY_CryptoHash(MTY_ALGORITHM_SHA256, CFDataGetBytePtr(bytes), CFDataGetLength(bytes),
		NULL, 0, buf, MTY_SHA256_SIZE);

	CFRelease(bytes);

	for (size_t x = 0; x < MTY_SHA256_SIZE; x++) {
		char append[8];
		snprintf(append, 8, "%02X:", buf[x]);
		MTY_Strcat(fingerprint, size, append);
	}

	// Remove the trailing ':'
	fingerprint[strlen(fingerprint) - 1] = '\0';
}

void MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size)
{
	if (!ctx->cert) {
		memset(fingerprint, 0, size);
		return;
	}

	tls_cert_to_fingerprint(ctx->cert, fingerprint, size);
}

void MTY_CertDestroy(MTY_Cert **cert)
{
	if (!cert || !*cert)
		return;

	MTY_Cert *ctx = *cert;

	if (ctx->public_key)
		CFRelease(ctx->public_key);

	if (ctx->private_key)
		CFRelease(ctx->private_key);

	if (ctx->cert)
		CFRelease(ctx->cert);

	if (ctx->ident)
		CFRelease(ctx->ident);

	MTY_Free(ctx);
	*cert = NULL;
}


// TLS, DTLS

static OSStatus tls_read_func(SSLConnectionRef connection, void *data, size_t *dataLength)
{
	MTY_TLS *ctx = (MTY_TLS *) connection;

	OSStatus r = noErr;

	if (ctx->offset < *dataLength) {
		*dataLength = ctx->offset;
		r = errSSLWouldBlock;
	}

	memcpy(data, ctx->buf, *dataLength);
	ctx->offset -= *dataLength;

	memmove(ctx->buf, ctx->buf + *dataLength, ctx->offset);

	return r;
}

static OSStatus tls_write_func(SSLConnectionRef connection, const void *data, size_t *dataLength)
{
	MTY_TLS *ctx = (MTY_TLS *) connection;

	// Encrypt
	if (ctx->w) {
		if (ctx->w->size < *dataLength)
			return errSSLBufferOverflow;

		memcpy(ctx->w->out, data, *dataLength);
		*ctx->w->written = *dataLength;

		return noErr;
	}

	// Handshake
	if (!ctx->write_func)
		return errSSLNetworkTimeout;

	if (!ctx->write_func(data, *dataLength, ctx->opaque))
		return errSSLNetworkTimeout;

	return noErr;
}

MTY_TLS *MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu)
{
	bool r = true;

	MTY_TLS *ctx = MTY_Alloc(1, sizeof(MTY_TLS));

	CFMutableArrayRef array = NULL;

	ctx->ctx = SSLCreateContext(NULL, kSSLClientSide, type == MTY_TLS_TYPE_DTLS ? kSSLDatagramType : kSSLStreamType);
	if (!ctx->ctx) {
		MTY_Log("'SSLCreateContext' failed");
		r = false;
		goto except;
	}

	OSStatus e = SSLSetProtocolVersionMin(ctx->ctx, type == MTY_TLS_TYPE_DTLS ? kDTLSProtocol12 : kTLSProtocol12);
	if (e != noErr) {
		MTY_Log("'SSLSetProtocolVersionMin' failed with error %d", e);
		r = false;
		goto except;
	}

	e = SSLSetIOFuncs(ctx->ctx, tls_read_func, tls_write_func);
	if (e != noErr) {
		MTY_Log("'SSLSetIOFuncs' failed with error %d", e);
		r = false;
		goto except;
	}

	e = SSLSetConnection(ctx->ctx, ctx);
	if (e != noErr) {
		MTY_Log("'SSLSetConnection' failed with error %d", e);
		r = false;
		goto except;
	}

	if (host) {
		e = SSLSetPeerDomainName(ctx->ctx, host, strlen(host));
		if (e != noErr) {
			MTY_Log("'SSLSetPeerDomainName' failed with error %d", e);
			r = false;
			goto except;
		}
	}

	if (type == MTY_TLS_TYPE_DTLS) {
		e = SSLSetMaxDatagramRecordSize(ctx->ctx, mtu + TLS_MTU_PADDING);
		if (e != noErr) {
			MTY_Log("'SSLSetMaxDatagramRecordSize' failed with error %d", e);
			r = false;
			goto except;
		}
	}

	if (cert) {
		array = CFArrayCreateMutable(NULL, 2, NULL);
		CFArrayAppendValue(array, cert->ident);
		CFArrayAppendValue(array, cert->cert);

		e = SSLSetCertificate(ctx->ctx, array);
		if (e != noErr) {
			MTY_Log("'SSLSetCertificate' failed with error %d", e);
			r = false;
			goto except;
		}
	}

	if (peerFingerprint)
		ctx->fp = MTY_Strdup(peerFingerprint);

	except:

	if (array)
		CFRelease(array);

	if (!r)
		MTY_TLSDestroy(&ctx);

	return ctx;
}

void MTY_TLSDestroy(MTY_TLS **tls)
{
	if (!tls || !*tls)
		return;

	MTY_TLS *ctx = *tls;

	if (ctx->ctx)
		CFRelease(ctx->ctx);

	MTY_Free(ctx->fp);
	MTY_Free(ctx->buf);

	MTY_Free(ctx);
	*tls = NULL;
}

static void tls_add_data(MTY_TLS *ctx, const void *buf, size_t size)
{
	if (size + ctx->offset > ctx->size) {
		ctx->size = size + ctx->offset;
		ctx->buf = MTY_Realloc(ctx->buf, ctx->size, 1);
	}

	memcpy(ctx->buf + ctx->offset, buf, size);
	ctx->offset += size;
}

MTY_Async MTY_TLSHandshake(MTY_TLS *ctx, const void *buf, size_t size, MTY_TLSWriteFunc writeFunc, void *opaque)
{
	if (buf && size > 0)
		tls_add_data(ctx, buf, size);

	ctx->opaque = opaque;
	ctx->write_func = writeFunc;

	OSStatus e = SSLHandshake(ctx->ctx);

	if (e != errSSLWouldBlock && e != noErr)
		MTY_Log("'SSLHandshake' failed with error %d", e);

	ctx->opaque = NULL;
	ctx->write_func = NULL;

	return e == errSSLWouldBlock ? MTY_ASYNC_CONTINUE : e == noErr ? MTY_ASYNC_OK : MTY_ASYNC_ERROR;
}

bool MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written)
{
	struct tls_write w = {0};
	w.out = out;
	w.size = outSize;
	w.written = written;

	ctx->w = &w;

	size_t processed = 0;
	OSStatus e = SSLWrite(ctx->ctx, in, inSize, &processed);
	if (e != noErr)
		MTY_Log("'SSLWrite' failed with error %d", e);

	ctx->w = NULL;

	return e == noErr;
}

bool MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read)
{
	tls_add_data(ctx, in, inSize);

	OSStatus e = SSLRead(ctx->ctx, out, outSize, read);

	bool success = e == noErr || e == errSSLWouldBlock;

	if (!success)
		MTY_Log("'SSLRead' failed with error %d", e);

	return success;
}
