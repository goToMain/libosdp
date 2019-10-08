/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE		/* See feature_test_macros(7) */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>

#include "osdp_common.h"
#include "aes.h"

int g_log_level = LOG_WARNING;	/* Note: log level is not contextual */

void osdp_set_log_level(int log_level)
{
	g_log_level = log_level;
}

void osdp_log(int log_level, const char *fmt, ...)
{
	va_list args;
	char *buf;
	const char *levels[] = {
		"EMERG", "ALERT", "CRIT ", "ERROR",
		"WARN ", "NOTIC", "INFO ", "DEBUG"
	};

	if (log_level > g_log_level)
		return;

	va_start(args, fmt);
	vasprintf(&buf, fmt, args);
	va_end(args);

	printf("OSDP: %s: %s\n", levels[log_level], buf);
	free(buf);
}

void osdp_dump(const char *head, const uint8_t * data, int len)
{
	int i;
	char str[16 + 1] = { 0 };

	printf("%s [%d] =>\n    0000  %02x ", head, len, len ? data[0] : 0);
	str[0] = isprint(data[0]) ? data[0] : '.';
	for (i = 1; i < len; i++) {
		if ((i & 0x0f) == 0) {
			printf(" |%16s|", str);
			printf("\n    %04d  ", i);
		} else if ((i & 0x07) == 0) {
			printf(" ");
		}
		printf("%02x ", data[i]);
		str[i & 0x0f] = isprint(data[i]) ? data[i] : '.';
	}
	if ((i &= 0x0f) != 0) {
		if (i < 8)
			printf(" ");
		for (; i < 16; i++) {
			printf("   ");
			str[i] = ' ';
		}
		printf(" |%16s|", str);
	}
	printf("\n");
}

uint16_t crc16_itu_t(uint16_t seed, const uint8_t * src, size_t len)
{
	for (; len > 0; len--) {
		seed = (seed >> 8U) | (seed << 8U);
		seed ^= *src++;
		seed ^= (seed & 0xffU) >> 4U;
		seed ^= seed << 12U;
		seed ^= (seed & 0xffU) << 5U;
	}
	return seed;
}

millis_t millis_now()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (millis_t) ((tv.tv_sec) * 1000L + (tv.tv_usec) / 1000L);
}

millis_t millis_since(millis_t last)
{
	return millis_now() - last;
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
		AES_init_ctx(&aes_ctx, key);
		AES_ECB_encrypt(&aes_ctx, data);
	}
}

void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	struct AES_ctx aes_ctx;

	if (iv != NULL) {
		AES_init_ctx_iv(&aes_ctx, key, iv);
		AES_CBC_decrypt_buffer(&aes_ctx, data, len);
	} else {
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
