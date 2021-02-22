// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <jni.h>

struct MTY_AESGCM {
	bool dummy;
};

MTY_AESGCM *MTY_AESGCMCreate(const void *key)
{
	bool r = false;
	MTY_AESGCM *ctx = MTY_Alloc(1, sizeof(MTY_AESGCM));

	if (!r)
		MTY_AESGCMDestroy(&ctx);

	return ctx;
}

bool MTY_AESGCMEncrypt(MTY_AESGCM *ctx, const void *nonce, const void *plainText, size_t size,
	void *hash, void *cipherText)
{
	return false;
}

bool MTY_AESGCMDecrypt(MTY_AESGCM *ctx, const void *nonce, const void *cipherText, size_t size,
	const void *hash, void *plainText)
{
	return false;
}

void MTY_AESGCMDestroy(MTY_AESGCM **aesgcm)
{
	if (!aesgcm || !*aesgcm)
		return;

	MTY_AESGCM *ctx = *aesgcm;

	MTY_Free(ctx);
	*aesgcm = NULL;
}
