/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE		/* See feature_test_macros(7) */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "osdp_common.h"
#include "osdp_aes.h"

#define LOG_CTX_GLOBAL -153

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.0.0"
#endif

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

const char *log_level_colors[LOG_MAX_LEVEL] = {
	RED,   RED,   RED,   RED,
	YEL,   MAG,   GRN,   GRN
};

const char *log_level_names[LOG_MAX_LEVEL] = {
	"EMERG", "ALERT", "CRIT ", "ERROR",
	"WARN ", "NOTIC", "INFO ", "DEBUG"
};

int g_log_level = LOG_WARNING;	/* Note: log level is not contextual */
int g_log_ctx = LOG_CTX_GLOBAL;
int g_old_log_ctx = LOG_CTX_GLOBAL;
int (*log_printf)(const char *fmt, ...) = printf;

void osdp_log_set_color(const char *color)
{
	if (isatty(fileno(stdout)))
		write(fileno(stdout), color, strlen(color));
}

void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...))
{
	g_log_level = log_level;
	if (log_fn != NULL)
		log_printf = log_fn;
}

void osdp_log_ctx_set(int log_ctx)
{
	g_old_log_ctx = g_log_ctx;
	g_log_ctx = log_ctx;
}

void osdp_log_ctx_reset()
{
	g_old_log_ctx = g_log_ctx;
	g_log_ctx = LOG_CTX_GLOBAL;
}

void osdp_log_ctx_restore()
{
	g_log_ctx = g_old_log_ctx;
}

void osdp_log(int log_level, const char *fmt, ...)
{
	va_list args;
	char *buf;

	if (log_level < LOG_EMERG || log_level >= LOG_MAX_LEVEL)
		return;

	if (log_level > g_log_level)
		return;

	va_start(args, fmt);
	vasprintf(&buf, fmt, args);
	va_end(args);
	osdp_log_set_color(log_level_colors[log_level]);
	if (g_log_ctx == LOG_CTX_GLOBAL)
		log_printf("OSDP: %s: %s\n", log_level_names[log_level], buf);
	else
		log_printf("OSDP: %s: PD[%d] %s\n", log_level_names[log_level],
			   g_log_ctx, buf);
	osdp_log_set_color(RESET);
	free(buf);
}

void osdp_dump(const char *head, const uint8_t *data, int len)
{
	int i;
	char str[16 + 1] = { 0 };

	osdp_log_set_color(RESET); /* no color for osdp_dump */
	log_printf("%s [%d] =>\n    0000  %02x ", head ? head : "", len,
	       len ? data[0] : 0);
	str[0] = isprint(data[0]) ? data[0] : '.';
	for (i = 1; i < len; i++) {
		if ((i & 0x0f) == 0) {
			log_printf(" |%16s|", str);
			log_printf("\n    %04d  ", i);
		} else if ((i & 0x07) == 0) {
			log_printf(" ");
		}
		log_printf("%02x ", data[i]);
		str[i & 0x0f] = isprint(data[i]) ? data[i] : '.';
	}
	if ((i &= 0x0f) != 0) {
		if (i < 8)
			log_printf(" ");
		for (; i < 16; i++) {
			log_printf("   ");
			str[i] = ' ';
		}
		log_printf(" |%16s|", str);
	}
	log_printf("\n");
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

uint16_t compute_crc16(const uint8_t *buf, size_t len)
{
	return crc16_itu_t(0x1D0F, buf, len);
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

void safe_free(void *p)
{
	if (p != NULL)
		free(p);
}

struct osdp_slab *osdp_slab_init(int block_size, int num_blocks)
{
	struct osdp_slab *slab;

	slab = calloc(1, sizeof(struct osdp_slab));
	if (slab == NULL)
		goto cleanup;

	slab->block_size = block_size + 2;
	slab->num_blocks = num_blocks;
	slab->free_blocks = num_blocks;
	slab->blob = calloc(num_blocks, block_size + 2);
	if (slab->blob == NULL)
		goto cleanup;

	return slab;

cleanup:
	osdp_slab_del(slab);
	return NULL;
}

void osdp_slab_del(struct osdp_slab *s)
{
	if (s != NULL) {
		safe_free(s->blob);
		safe_free(s);
	}
}

void *osdp_slab_alloc(struct osdp_slab *s)
{
	int i;
	uint8_t *p = NULL;

	for (i = 0; i < s->num_blocks; i++) {
		p = s->blob + (s->block_size * i);
		if (*p == 0)
			break;
	}
	if (p == NULL || i == s->num_blocks)
		return NULL;
	*p = 1; // Mark as allocated.
	*(p + s->block_size - 1) = 0x55;  // cookie.
	s->free_blocks -= 1;
	return (void *)(p + 1);
}

void osdp_slab_free(struct osdp_slab *s, void *block)
{
	uint8_t *p = block;

	if (*(p + s->block_size - 2) != 0x55) {
		LOG_EM("slab: fatal, cookie crushed!");
		exit(0);
	}
	if (*(p - 1) != 1) {
		LOG_EM("slab: free called on unallocated block");
		return;
	}
	s->free_blocks += 1;
	*(p - 1) = 0; // Mark as free.
	// memset(p - 1, 0, s->block_size);
}

int osdp_slab_blocks(struct osdp_slab *s)
{
	return (int)s->free_blocks;
}

/* --- Exported Methods --- */

const char *osdp_get_version()
{
	return PROJECT_NAME "-" PROJECT_VERSION;
}

const char *osdp_get_source_info()
{
	if (strlen(GIT_TAG) > 0)
		return GIT_BRANCH " (" GIT_TAG ")";
	else if (strlen(GIT_REV) > 0)
		return GIT_BRANCH " (" GIT_REV GIT_DIFF ")";
	else
		return GIT_BRANCH;
}

uint32_t osdp_get_sc_status_mask(osdp_t *ctx)
{
	int i;
	uint32_t mask = 0;
	struct osdp_pd *pd;

	assert(ctx);

	if (isset_flag(to_osdp(ctx), FLAG_CP_MODE)) {
		for (i = 0; i < to_osdp(ctx)->cp->num_pd; i++) {
			pd = to_pd(ctx, i);
			if (pd->state == CP_STATE_ONLINE &&
			    isset_flag(pd, PD_FLAG_SC_ACTIVE)) {
				mask |= 1 << i;
			}
		}
	} else {
		pd = to_pd(ctx, 0);
		if (isset_flag(pd, PD_FLAG_SC_ACTIVE)) {
			mask = 1;
		}
	}

	return mask;
}

uint32_t osdp_get_status_mask(osdp_t *ctx)
{
	int i;
	uint32_t mask = 0;
	struct osdp_pd *pd;

	assert(ctx);

	if (isset_flag(to_osdp(ctx), FLAG_CP_MODE)) {
		for (i = 0; i < to_osdp(ctx)->cp->num_pd; i++) {
			pd = to_pd(ctx, i);
			if (pd->state == CP_STATE_ONLINE) {
				mask |= 1 << i;
			}
		}
	} else {
		/* PD is stateless */
		mask = 1;
	}

	return mask;
}
