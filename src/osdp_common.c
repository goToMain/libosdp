/*
 * Copyright (c) 2019-2022 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE /* See feature_test_macros(7) */

#include <stdarg.h>
#include <stdlib.h>
#ifndef CONFIG_DISABLE_PRETTY_LOGGING
#include <unistd.h>
#endif
#include <sys/time.h>

#include "osdp_common.h"

#ifdef CONFIG_DISABLE_PRETTY_LOGGING
LOGGER_DEFINE(osdp, LOG_INFO, LOGGER_FLAG_NO_COLORS);
#else
LOGGER_DEFINE(osdp, LOG_INFO, LOGGER_FLAG_NONE);
#endif

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

const char *osdp_cmd_name(int cmd_id)
{
	static const char * const names[] = {
		[CMD_POLL         - CMD_POLL] = "POLL",
		[CMD_ID           - CMD_POLL] = "ID",
		[CMD_CAP          - CMD_POLL] = "CAP",
		[CMD_DIAG         - CMD_POLL] = "DIAG",
		[CMD_LSTAT        - CMD_POLL] = "LSTAT",
		[CMD_ISTAT        - CMD_POLL] = "ISTAT",
		[CMD_OSTAT        - CMD_POLL] = "OSTAT",
		[CMD_RSTAT        - CMD_POLL] = "RSTAT",
		[CMD_OUT          - CMD_POLL] = "OUT",
		[CMD_LED          - CMD_POLL] = "LED",
		[CMD_BUZ          - CMD_POLL] = "BUZ",
		[CMD_TEXT         - CMD_POLL] = "TEXT",
		[CMD_RMODE        - CMD_POLL] = "RMODE",
		[CMD_TDSET        - CMD_POLL] = "TDSET",
		[CMD_COMSET       - CMD_POLL] = "COMSET",
		[CMD_DATA         - CMD_POLL] = "DATA",
		[CMD_XMIT         - CMD_POLL] = "XMIT",
		[CMD_PROMPT       - CMD_POLL] = "PROMPT",
		[CMD_SPE          - CMD_POLL] = "SPE",
		[CMD_BIOREAD      - CMD_POLL] = "BIOREAD",
		[CMD_BIOMATCH     - CMD_POLL] = "BIOMATCH",
		[CMD_KEYSET       - CMD_POLL] = "KEYSET",
		[CMD_CHLNG        - CMD_POLL] = "CHLNG",
		[CMD_SCRYPT       - CMD_POLL] = "SCRYPT",
		[CMD_CONT         - CMD_POLL] = "CONT",
		[CMD_ABORT        - CMD_POLL] = "ABORT",
		[CMD_FILETRANSFER - CMD_POLL] = "FILETRANSFER",
		[CMD_ACURXSIZE    - CMD_POLL] = "ACURXSIZE",
		[CMD_MFG          - CMD_POLL] = "MFG",
		[CMD_SCDONE       - CMD_POLL] = "SCDONE",
		[CMD_XWR          - CMD_POLL] = "XWR",
		[CMD_KEEPACTIVE   - CMD_POLL] = "KEEPACTIVE",
	};

	if (cmd_id < CMD_POLL || cmd_id > CMD_KEEPACTIVE) {
		return NULL;
	}
	return names[cmd_id - CMD_POLL];
}

const char *osdp_reply_name(int reply_id)
{
	static const char * const names[] = {
		[REPLY_ACK       - REPLY_ACK] = "ACK",
		[REPLY_NAK       - REPLY_ACK] = "NAK",
		[REPLY_PDID      - REPLY_ACK] = "PDID",
		[REPLY_PDCAP     - REPLY_ACK] = "PDCAP",
		[REPLY_LSTATR    - REPLY_ACK] = "LSTATR",
		[REPLY_ISTATR    - REPLY_ACK] = "ISTATR",
		[REPLY_OSTATR    - REPLY_ACK] = "OSTATR",
		[REPLY_RSTATR    - REPLY_ACK] = "RSTATR",
		[REPLY_RAW       - REPLY_ACK] = "RAW",
		[REPLY_FMT       - REPLY_ACK] = "FMT",
		[REPLY_PRES      - REPLY_ACK] = "PRES",
		[REPLY_KEYPPAD   - REPLY_ACK] = "KEYPPAD",
		[REPLY_COM       - REPLY_ACK] = "COM",
		[REPLY_SCREP     - REPLY_ACK] = "SCREP",
		[REPLY_SPER      - REPLY_ACK] = "SPER",
		[REPLY_BIOREADR  - REPLY_ACK] = "BIOREADR",
		[REPLY_BIOMATCHR - REPLY_ACK] = "BIOMATCHR",
		[REPLY_CCRYPT    - REPLY_ACK] = "CCRYPT",
		[REPLY_RMAC_I    - REPLY_ACK] = "RMAC_I",
		[REPLY_FTSTAT    - REPLY_ACK] = "FTSTAT",
		[REPLY_MFGREP    - REPLY_ACK] = "MFGREP",
		[REPLY_BUSY      - REPLY_ACK] = "BUSY",
		[REPLY_XRD       - REPLY_ACK] = "XRD",
	};

	if (reply_id < REPLY_ACK || reply_id > REPLY_XRD) {
		return NULL;
	}
	return names[reply_id - REPLY_ACK];
}

/* --- Exported Methods --- */

OSDP_EXPORT
void osdp_logger_init(int log_level, osdp_log_puts_fn_t log_fn)
{
	logger_t *ctx = get_log_ctx(osdp);

	logger_set_log_level(ctx, log_level);
	if (log_fn) {
		logger_set_put_fn(ctx, log_fn);
		logger_set_file(ctx, NULL);
	} else {
		logger_set_put_fn(ctx, NULL);
		logger_set_file(ctx, stderr);
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
void osdp_get_sc_status_mask(osdp_t *ctx, uint8_t *bitmask)
{
	input_check(ctx);
	int i, pos;
	uint8_t *mask = bitmask;
	struct osdp_pd *pd;

	*mask = 0;
	for (i = 0; i < NUM_PD(ctx); i++) {
		pos = i & 0x07;
		if (i && pos == 0) {
			mask++;
			*mask = 0;
		}
		pd = osdp_to_pd(ctx, i);
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
			*mask |= 1 << pos;
		}
	}
}

OSDP_EXPORT
void osdp_get_status_mask(osdp_t *ctx, uint8_t *bitmask)
{
	input_check(ctx);
	int i, pos;
	uint8_t *mask = bitmask;
	struct osdp_pd *pd;

	*mask = 0;
	for (i = 0; i < NUM_PD(ctx); i++) {
		pos = i & 0x07;
		if (i && pos == 0) {
			mask++;
			*mask = 0;
		}
		pd = osdp_to_pd(ctx, i);
		if (ISSET_FLAG(pd, PD_FLAG_PD_MODE) ||
		    pd->state == OSDP_CP_STATE_ONLINE) {
			*mask |= 1 << pos;
		}
	}
}

OSDP_EXPORT
void osdp_set_command_complete_callback(osdp_t *ctx,
					osdp_command_complete_callback_t cb)
{
	input_check(ctx);

	TO_OSDP(ctx)->command_complete_callback = cb;
}
