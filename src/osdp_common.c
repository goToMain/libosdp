/*
 * Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE /* See feature_test_macros(7) */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef CONFIG_DISABLE_PRETTY_LOGGING
#include <unistd.h>
#endif
#include <sys/time.h>

#include "osdp_common.h"

#define LOG_CTX_GLOBAL -153
#define LOG_TAG	       "COM: "
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

const char *log_level_names[LOG_MAX_LEVEL] = { "EMERG", "ALERT", "CRIT ",
					       "ERROR", "WARN ", "NOTIC",
					       "INFO ", "DEBUG" };

int g_log_level = LOG_WARNING; /* Note: log level is not contextual */
int g_log_ctx = LOG_CTX_GLOBAL;
int g_old_log_ctx = LOG_CTX_GLOBAL;
osdp_log_fn_t log_printf;

void osdp_log_set_colour(int log_level)
{
#ifndef CONFIG_DISABLE_PRETTY_LOGGING
	int ret, len;
	const char *colour;
	static const char *colours[LOG_MAX_LEVEL] = { RED, RED, RED, RED,
						      YEL, MAG, GRN, RESET };

	colour = (log_level < 0) ? RESET : colours[log_level];
	len = strnlen(colour, 8);
	if (isatty(fileno(stdout))) {
		ret = write(fileno(stdout), colour, len);
		assert(ret == len);
		ARG_UNUSED(ret); /* squash warning in Release builds */
	}
#else
	ARG_UNUSED(log_level);
#endif
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
	size_t len;
	va_list args;
	static char buf[128];

	if (log_printf == NULL || log_level >= LOG_MAX_LEVEL ||
	    log_level > g_log_level) {
		return;
	}

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	assert(len < sizeof(buf));
	if (log_level < 0) {
		log_printf("OSDP: %s\n", buf);
		return;
	}
	osdp_log_set_colour(log_level);
	if (g_log_ctx == LOG_CTX_GLOBAL) {
		log_printf("OSDP: %s: %s\n", log_level_names[log_level], buf);
	} else {
		log_printf("OSDP: %s: PD[%d]: %s\n", log_level_names[log_level],
			   g_log_ctx, buf);
	}
	osdp_log_set_colour(-1); /* Reset colour */
}

uint16_t crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len)
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

uint16_t osdp_compute_crc16(const uint8_t *buf, size_t len)
{
	return crc16_itu_t(0x1D0F, buf, len);
}

int64_t osdp_millis_now()
{
	return millis_now();
}

int64_t osdp_millis_since(int64_t last)
{
	return osdp_millis_now() - last;
}

/* --- Exported Methods --- */

OSDP_EXPORT
void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...))
{
	g_log_level = log_level;
	if (log_fn != NULL) {
		log_printf = log_fn;
	}
}

OSDP_EXPORT
const char *osdp_get_version()
{
	return PROJECT_NAME "-" PROJECT_VERSION;
}

OSDP_EXPORT
const char *osdp_get_source_info()
{
	if (strnlen(GIT_TAG, 8) > 0) {
		return GIT_BRANCH " (" GIT_TAG ")";
	} else if (strnlen(GIT_REV, 8) > 0) {
		return GIT_BRANCH " (" GIT_REV GIT_DIFF ")";
	} else {
		return GIT_BRANCH;
	}
}

OSDP_EXPORT
uint32_t osdp_get_sc_status_mask(osdp_t *ctx)
{
	input_check(ctx);
	int i;
	uint32_t mask = 0;
	struct osdp_pd *pd;

	if (ISSET_FLAG(TO_OSDP(ctx), FLAG_CP_MODE)) {
		for (i = 0; i < NUM_PD(ctx); i++) {
			pd = TO_PD(ctx, i);
			if (pd->state == OSDP_CP_STATE_ONLINE &&
			    ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
				mask |= 1 << i;
			}
		}
	} else {
		pd = TO_PD(ctx, 0);
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
			mask = 1;
		}
	}

	return mask;
}

OSDP_EXPORT
uint32_t osdp_get_status_mask(osdp_t *ctx)
{
	input_check(ctx);
	int i;
	uint32_t mask = 0;
	struct osdp_pd *pd;

	if (ISSET_FLAG(TO_OSDP(ctx), FLAG_CP_MODE)) {
		for (i = 0; i < TO_OSDP(ctx)->cp->num_pd; i++) {
			pd = TO_PD(ctx, i);
			if (pd->state == OSDP_CP_STATE_ONLINE) {
				mask |= 1 << i;
			}
		}
	} else {
		/* PD is stateless */
		mask = 1;
	}

	return mask;
}

OSDP_EXPORT
void osdp_set_command_complete_callback(osdp_t *ctx,
					osdp_command_complete_callback_t cb)
{
	input_check(ctx);

	TO_OSDP(ctx)->command_complete_callback = cb;
}
