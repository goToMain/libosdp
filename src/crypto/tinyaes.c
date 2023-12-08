/*
 * Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "tinyaes_src.h"

void osdp_crypt_setup()
{
}

void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	struct AES_ctx aes_ctx;

	if (iv != NULL) {
		/* encrypt multiple block with AES in CBC mode */
		AES_init_ctx_iv(&aes_ctx, key, iv);
		AES_CBC_encrypt_buffer(&aes_ctx, data, len);
	} else {
		/* encrypt one block with AES in ECB mode */
		assert(len <= 16);
		AES_init_ctx(&aes_ctx, key);
		AES_ECB_encrypt(&aes_ctx, data);
	}
}

void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	struct AES_ctx aes_ctx;

	if (iv != NULL) {
		/* decrypt multiple block with AES in CBC mode */
		AES_init_ctx_iv(&aes_ctx, key, iv);
		AES_CBC_decrypt_buffer(&aes_ctx, data, len);
	} else {
		/* decrypt one block with AES in ECB mode */
		assert(len <= 16);
		AES_init_ctx(&aes_ctx, key);
		AES_ECB_decrypt(&aes_ctx, data);
	}
}

void osdp_fill_random(uint8_t *buf, int len)
{
	int i, rnd;

	for (i = 0; i < len; i++) {
		rnd = rand();
		buf[i] = (uint8_t)(((float)rnd) / RAND_MAX * 256);
	}
}

void osdp_crypt_teardown()
{
}