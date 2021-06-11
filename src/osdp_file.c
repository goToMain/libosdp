/*
 * Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * The following define is just to cheat IDEs; normally it is provided by build
 * system and this file is not complied if not configured.
 */
#ifndef CONFIG_OSDP_FILE
#define CONFIG_OSDP_FILE
#endif

#include <stdlib.h>

#include "osdp_file.h"

#define LOG_TAG "FOP: "

int osdp_file_cmd_tx_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	ssize_t rc;
	struct osdp_cmd_file_xfer *p = (struct osdp_cmd_file_xfer *)buf;
	struct osdp_file *f = TO_FILE(pd);

	if ((size_t)max_len <= sizeof(struct osdp_cmd_file_xfer)) {
		return -1;
	}

	p->type = f->file_id;
	p->offset = f->offset;
	p->size = f->size;
	p->length = max_len - sizeof(struct osdp_cmd_file_xfer); // TODO

	rc = f->ops->read(p->type, p->data, p->length, p->offset);
	if (rc < 0) {
		return -1;
	}
	f->last_send = rc;
	return sizeof(struct osdp_cmd_file_xfer) + rc;
}

int osdp_file_cmd_tx_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	ssize_t rc;
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_xfer *p = (struct osdp_cmd_file_xfer *)buf;

	if ((size_t)len < sizeof(struct osdp_cmd_file_xfer) + p->length) {
		return -1;
	}

	if (p->offset == 0) {
		/* new file write request */
		if (f->ops->open(p->type, &f->size) < 0) {
			LOG_ERR("Open for FD:%d failed!", p->type);
			return -1;
		}
		f->state = OSDP_FILE_INPROG;
	}

	rc = f->ops->write(p->type, p->data, p->length, p->offset);
	if (rc < 0) {
		return -1;
	}
	return 0;
}

int osdp_file_cmd_stat_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	struct osdp_cmd_file_stat *p = (struct osdp_cmd_file_stat *)buf;
	struct osdp_file *f = TO_FILE(pd);

	if ((size_t)max_len <= sizeof(struct osdp_cmd_file_stat)) {
		return -1;
	}

	p->status = (f->last_send > 0) ? 0: -1;
	p->rx_size = 0;
	p->control = 0;
	p->delay = 0;

	return sizeof(struct osdp_cmd_file_stat);
}

int osdp_file_cmd_stat_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_stat *p = (struct osdp_cmd_file_stat *)buf;

	if ((size_t)len < sizeof(struct osdp_cmd_file_stat)) {
		return -1;
	}

	if (p->status == 0 || p->status == 1) {
		f->offset += f->last_send;
	}

	return 0;
}

/**
 * Return true here to queue a CMD_FILETRANSFER. This function is a critical
 * path so keep it simple.
 */
bool osdp_file_tx_pending(struct osdp_pd *pd)
{
	struct osdp_file *f = TO_FILE(pd);

	if (!f || f->state == OSDP_FILE_IDLE) {
		return false;
	}

	return true;
}

/* Entry point based on OSDP_CMD_FILE kick off a file transfer */
int osdp_file_tx_initiate(struct osdp_pd *pd, int file_id, uint32_t flags)
{
	struct osdp_file *f = TO_FILE(pd);
	ARG_UNUSED(flags);

	if (!f || !f->ops) {
		LOG_WRN("File ops not registered!");
		return -1;
	}
	f->state = OSDP_FILE_INPROG;
	f->file_id = file_id;
	return 0;
}

/* --- Exported Methods --- */

OSDP_EXPORT
int osdp_file_register_ops(osdp_t *ctx, int pd, struct osdp_file_ops *ops)
{
	input_check(ctx, pd);
	struct osdp_pd *pd_ctx = TO_PD(ctx, pd);

	if (!pd_ctx->file) {
		pd_ctx->file = calloc(1, sizeof(struct osdp_file));
		if (pd_ctx->file == NULL) {
			LOG_ERR("Failed to alloc struct osdp_file");
			return -1;
		}
	}

	pd_ctx->file->ops = ops;
	return 0;
}

OSDP_EXPORT
int osdp_file_tx_status(osdp_t *ctx, int pd, size_t *size, size_t *offset)
{
	input_check(ctx, pd);
	struct osdp_file *f = TO_FILE(TO_PD(ctx, pd));

	if (f->state != OSDP_FILE_INPROG) {
		return -1;
	}

	*size = f->size;
	*offset = f->offset;
	return 0;
}
