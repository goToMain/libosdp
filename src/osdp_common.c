/*
 * Copyright (c) 2019-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdarg.h>
#include <stdlib.h>
#ifndef CONFIG_DISABLE_PRETTY_LOGGING
#include <unistd.h>
#endif
#include <sys/time.h>

#include "osdp_common.h"

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

int64_t osdp_millis_now(void)
{
	return millis_now();
}

int64_t osdp_millis_since(int64_t last)
{
	return osdp_millis_now() - last;
}

const char *osdp_cmd_name(int cmd_id)
{
	const char *name;
	static const char * const names[] = {
		[CMD_POLL         - CMD_POLL] = "POLL",
		[CMD_ID           - CMD_POLL] = "ID",
		[CMD_CAP          - CMD_POLL] = "CAP",
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
		[CMD_BIOREAD      - CMD_POLL] = "BIOREAD",
		[CMD_BIOMATCH     - CMD_POLL] = "BIOMATCH",
		[CMD_KEYSET       - CMD_POLL] = "KEYSET",
		[CMD_CHLNG        - CMD_POLL] = "CHLNG",
		[CMD_SCRYPT       - CMD_POLL] = "SCRYPT",
		[CMD_ACURXSIZE    - CMD_POLL] = "ACURXSIZE",
		[CMD_FILETRANSFER - CMD_POLL] = "FILETRANSFER",
		[CMD_MFG          - CMD_POLL] = "MFG",
		[CMD_XWR          - CMD_POLL] = "XWR",
		[CMD_ABORT        - CMD_POLL] = "ABORT",
		[CMD_PIVDATA      - CMD_POLL] = "PIVDATA",
		[CMD_CRAUTH       - CMD_POLL] = "CRAUTH",
		[CMD_GENAUTH      - CMD_POLL] = "GENAUTH",
		[CMD_KEEPACTIVE   - CMD_POLL] = "KEEPACTIVE",
	};

	if (cmd_id < CMD_POLL || cmd_id > CMD_KEEPACTIVE) {
		return "INVALID";
	}
	name = names[cmd_id - CMD_POLL];
	if (name[0] == '\0') {
		return "UNKNOWN";
	}
	return name;
}

const char *osdp_reply_name(int reply_id)
{
	const char *name;
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
		[REPLY_KEYPPAD   - REPLY_ACK] = "KEYPPAD",
		[REPLY_COM       - REPLY_ACK] = "COM",
		[REPLY_BIOREADR  - REPLY_ACK] = "BIOREADR",
		[REPLY_BIOMATCHR - REPLY_ACK] = "BIOMATCHR",
		[REPLY_CCRYPT    - REPLY_ACK] = "CCRYPT",
		[REPLY_RMAC_I    - REPLY_ACK] = "RMAC_I",
		[REPLY_FTSTAT    - REPLY_ACK] = "FTSTAT",
		[REPLY_MFGREP    - REPLY_ACK] = "MFGREP",
		[REPLY_BUSY      - REPLY_ACK] = "BUSY",
		[REPLY_PIVDATAR  - REPLY_ACK] = "PIVDATA",
		[REPLY_CRAUTHR   - REPLY_ACK] = "CRAUTH",
		[REPLY_MFGSTATR  - REPLY_ACK] = "MFGSTATR",
		[REPLY_MFGERRR   - REPLY_ACK] = "MFGERR",
		[REPLY_XRD       - REPLY_ACK] = "XRD",
	};

	if (reply_id < REPLY_ACK || reply_id > REPLY_XRD) {
		return "INVALID";
	}
	name = names[reply_id - REPLY_ACK];
	if (!name) {
		return "UNKNOWN";
	}
	return name;
}

void osdp_keyset_complete(struct osdp_pd *pd)
{
	cp_keyset_complete(pd, true);
}

int osdp_rb_push(struct osdp_rb *p, uint8_t data)
{
	size_t next;

	next = p->head + 1;
	if (next >= sizeof(p->buffer))
		next = 0;

	if (next == p->tail)
		return -1;

	p->buffer[p->head] = data;
	p->head = next;
	return 0;
}

int osdp_rb_push_buf(struct osdp_rb *p, uint8_t *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (osdp_rb_push(p, buf[i])) {
			break;
		}
	}

	return i;
}

int osdp_rb_pop(struct osdp_rb *p, uint8_t *data)
{
	size_t next;

	if (p->head == p->tail)
		return -1;

	next = p->tail + 1;
	if (next >= sizeof(p->buffer))
		next = 0;

	*data = p->buffer[p->tail];
	p->tail = next;
	return 0;
}

int osdp_rb_pop_buf(struct osdp_rb *p, uint8_t *buf, int max_len)
{
	int i;

	for (i = 0; i < max_len; i++) {
		if (osdp_rb_pop(p, buf + i)) {
			break;
		}
	}

	return i;
}

/* --- Exported Methods --- */

OSDP_EXPORT
void osdp_logger_init(const char *name, int log_level,
		       osdp_log_puts_fn_t log_fn)
{
	logger_t ctx;
	FILE *file = NULL;
	int flags = LOGGER_FLAG_NONE;

#ifdef CONFIG_DISABLE_PRETTY_LOGGING
	flags |= LOGGER_FLAG_NO_COLORS;
#endif
	if (!log_fn)
		file = stderr;

	logger_init(&ctx, log_level, name, REPO_ROOT, log_fn, file, NULL, flags);
	logger_set_default(&ctx); /* Mark this config as logging default */
}

OSDP_EXPORT
void osdp_set_log_callback(osdp_log_callback_fn_t cb)
{
	logger_t ctx;
	int flags = LOGGER_FLAG_NONE;

	logger_init(&ctx, 0, NULL, REPO_ROOT, NULL, NULL, cb, flags);
	logger_set_default(&ctx); /* Mark this config as logging default */
}

OSDP_EXPORT
const char *osdp_get_version()
{
	return PROJECT_VERSION;
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
void osdp_get_sc_status_mask(const osdp_t *ctx, uint8_t *bitmask)
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
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE) &&
		    !ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			*mask |= 1 << pos;
		}
	}
}

OSDP_EXPORT
void osdp_get_status_mask(const osdp_t *ctx, uint8_t *bitmask)
{
	input_check(ctx);
	int i, pos;
	uint8_t *mask = bitmask;
	struct osdp_pd *pd = osdp_to_pd(ctx, 0);

	if (ISSET_FLAG(pd, PD_FLAG_PD_MODE)) {
		*mask = osdp_millis_since(pd->tstamp) < OSDP_PD_ONLINE_TOUT_MS;
		return;
	}

	*mask = 0;
	for (i = 0; i < NUM_PD(ctx); i++) {
		pos = i & 0x07;
		if (i && pos == 0) {
			mask++;
			*mask = 0;
		}
		pd = osdp_to_pd(ctx, i);
		if (pd->state == OSDP_CP_STATE_ONLINE) {
			*mask |= 1 << pos;
		}
	}
}
