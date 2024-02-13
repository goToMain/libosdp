/*
 * Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "osdp_file.h"

static inline void file_state_reset(struct osdp_file *f)
{
	f->flags = 0;
	f->offset = 0;
	f->length = 0;
	f->errors = 0;
	f->size = 0;
	f->state = OSDP_FILE_IDLE;
	f->file_id = 0;
	f->cancel_req = false;
}

static inline bool file_tx_in_progress(struct osdp_file *f)
{
	return f && f->state == OSDP_FILE_INPROG;
}

/* --- Sender CMD/RESP Handers --- */

int osdp_file_cmd_tx_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int buf_available;
	struct osdp_cmd_file_xfer *p = (struct osdp_cmd_file_xfer *)buf;
	struct osdp_file *f = TO_FILE(pd);

	if (f == NULL) {
		LOG_ERR("TX_Build: File ops not registered!");
		return -1;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("TX_Build: File transfer is not in progress!");
		return -1;
	}

	if ((size_t)max_len <= sizeof(struct osdp_cmd_file_xfer)) {
		LOG_ERR("TX_Build: insufficient space need:%zu have:%d",
			sizeof(struct osdp_cmd_file_xfer), max_len);
		return -1;
	}

	/**
	 * OSDP File module is a bit different than the rest of LibOSDP: it
	 * tries to greedily consume all available packet space. We need to
	 * account for the bytes that phy layer would add and then account for
	 * the overhead due to encryption if a secure channel is active. For
	 * now 16 is choosen based on crude observation.
	 *
	 * TODO: Try to add smarts here later.
	 */
	buf_available = max_len - sizeof(struct osdp_cmd_file_xfer) - 16;

	p->type = f->file_id;
	p->offset = f->offset;
	p->size = f->size;

	f->length = f->ops.read(f->ops.arg, p->data, buf_available, p->offset);
	if (f->length < 0) {
		LOG_ERR("TX_Build: user read failed! rc:%d len:%d off:%d",
			f->length, buf_available, p->offset);
		f->errors++;
		f->length = 0;
		return -1;
	}
	if (f->length == 0) {
		LOG_WRN("TX_Build: Read 0 length chunk; Aborting transfer!");
		file_state_reset(f);
		return -1;
	}

	p->length = f->length;

	return sizeof(struct osdp_cmd_file_xfer) + f->length;
}

int osdp_file_cmd_stat_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_stat *p = (struct osdp_cmd_file_stat *)buf;

	if (f == NULL) {
		LOG_ERR("Stat_Decode: File ops not registered!");
		return -1;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("Stat_Decode: File transfer is not in progress!");
		return -1;
	}

	if ((size_t)len < sizeof(struct osdp_cmd_file_stat)) {
		LOG_ERR("Stat_Decode: invalid decode len:%d exp:%zu",
			len, sizeof(struct osdp_cmd_file_stat));
		return -1;
	}

	if (p->status == 0) {
		f->offset += f->length;
		f->errors = 0;
	} else {
		f->errors++;
	}
	f->length = 0;

	assert(f->offset <= f->size);
	if (f->offset == f->size) { /* EOF */
		if (f->ops.close(f->ops.arg) < 0) {
			LOG_ERR("Stat_Decode: Close failed!");
			return -1;
		}
		f->state = OSDP_FILE_DONE;
		LOG_INF("Stat_Decode: File transfer complete");
	}

	return 0;
}

/* --- Receiver CMD/RESP Handler --- */

int osdp_file_cmd_tx_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int rc;
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_xfer *p = (struct osdp_cmd_file_xfer *)buf;
	struct osdp_cmd cmd;

	if (f == NULL) {
		LOG_ERR("TX_Decode: File ops not registered!");
		return -1;
	}

	if (f->state == OSDP_FILE_IDLE || f->state == OSDP_FILE_DONE) {
		if (pd->command_callback) {
			/**
			 * Notify app of this command and make sure
			 * we can proceed
			 */
			cmd.id = OSDP_CMD_FILE_TX;
			cmd.file_tx.flags = f->flags;
			cmd.file_tx.id = p->type;
			rc = pd->command_callback(pd->command_callback_arg, &cmd);
			if (rc < 0)
				return -1;
		}

		/* new file write request */
		int size = (int)p->size;
		if (f->ops.open(f->ops.arg, p->type, &size) < 0) {
			LOG_ERR("TX_Decode: Open failed! fd:%d", p->type);
			return -1;
		}

		LOG_INF("TX_Decode: Starting file transfer");
		file_state_reset(f);
		f->file_id = p->type;
		f->size = size;
		f->state = OSDP_FILE_INPROG;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("TX_Decode: File transfer is not in progress!");
		return -1;
	}

	if ((size_t)len <= sizeof(struct osdp_cmd_file_xfer)) {
		LOG_ERR("TX_Decode: invalid decode len:%d exp>=%zu",
			len, sizeof(struct osdp_cmd_file_xfer));
		return -1;
	}

	f->length = f->ops.write(f->ops.arg, p->data, p->length, p->offset);
	if (f->length != p->length) {
		LOG_ERR("TX_Decode: user write failed! rc:%d len:%d off:%d",
			f->length, p->length, p->offset);
		f->errors++;
		return -1;
	}

	return 0;
}

int osdp_file_cmd_stat_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	struct osdp_cmd_file_stat *p = (struct osdp_cmd_file_stat *)buf;
	struct osdp_file *f = TO_FILE(pd);

	if (f == NULL) {
		LOG_ERR("Stat_Build: File ops not registered!");
		return -1;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("Stat_Build: File transfer is not in progress!");
		return -1;
	}

	if ((size_t)max_len < sizeof(struct osdp_cmd_file_stat)) {
		LOG_ERR("Stat_Build: insufficient space need:%zu have:%d",
			sizeof(struct osdp_cmd_file_stat), max_len);
		return -1;
	}

	if (f->length > 0) {
		p->status = 0;
		f->errors = 0;
		f->offset += f->length;
	} else {
		p->status = -1;
	}

	p->rx_size = 0;
	p->control = 0;
	p->delay = 0;
	f->length = 0;

	assert(f->offset <= f->size);
	if (f->offset == f->size) { /* EOF */
		if (f->ops.close(f->ops.arg) < 0) {
			LOG_ERR("Stat_Build: Close failed!");
			return -1;
		}
		f->state = OSDP_FILE_DONE;
		LOG_INF("TX_Decode: File receive complete");
	}

	return sizeof(struct osdp_cmd_file_stat);
}

/* --- State Management --- */

void osdp_file_tx_abort(struct osdp_pd *pd)
{
	struct osdp_file *f = TO_FILE(pd);

	if (file_tx_in_progress(f)) {
		f->ops.close(f->ops.arg);
		file_state_reset(f);
	}
}

int osdp_get_file_tx_state(struct osdp_pd *pd)
{
	struct osdp_file *f = TO_FILE(pd);

	if (!f || f->state == OSDP_FILE_IDLE || f->state == OSDP_FILE_DONE) {
		return OSDP_FILE_TX_STATE_IDLE;
	}

	if (f->errors > OSDP_FILE_ERROR_RETRY_MAX || f->cancel_req) {
		LOG_ERR("Aborting transfer of file fd:%d", f->file_id);
		osdp_file_tx_abort(pd);
		return OSDP_FILE_TX_STATE_ERROR;
	}

	return OSDP_FILE_TX_STATE_PENDING;
}

/**
 * Entry point based on command OSDP_CMD_FILE to kick off a new file transfer.
 */
int osdp_file_tx_command(struct osdp_pd *pd, int file_id, uint32_t flags)
{
	int size = 0;
	struct osdp_file *f = TO_FILE(pd);

	if (f == NULL) {
		LOG_ERR("TX_init: File ops not registered!");
		return -1;
	}

	if (file_tx_in_progress(f)) {
		if (flags & OSDP_CMD_FILE_TX_FLAG_CANCEL) {
			if (file_id == f->file_id) {
				f->cancel_req = true;
				return 0;
			}
			LOG_ERR("TX_init: invalid cancel request; no such tx!");
			return -1;
		}
		LOG_ERR("TX_init: A file tx is already in progress");
		return -1;
	}

	if (flags & OSDP_CMD_FILE_TX_FLAG_CANCEL) {
		LOG_ERR("TX_init: invalid cancel request");
		return -1;
	}

	if (f->ops.open(f->ops.arg, file_id, &size) < 0) {
		LOG_ERR("TX_init: Open failed! fd:%d", file_id);
		return -1;
	}

	if (size <= 0) {
		LOG_ERR("TX_init: Invalid file size %d", size);
		return -1;
	}

	LOG_INF("TX_init: Starting file transfer of size: %d", size);

	file_state_reset(f);
	f->flags = flags;
	f->file_id = file_id;
	f->size = size;
	f->state = OSDP_FILE_INPROG;
	return 0;
}

/* --- Exported Methods --- */

OSDP_EXPORT
int osdp_file_register_ops(osdp_t *ctx, int pd_idx,
			   const struct osdp_file_ops *ops)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	if (!pd->file) {
		pd->file = calloc(1, sizeof(struct osdp_file));
		if (pd->file == NULL) {
			LOG_PRINT("Failed to alloc struct osdp_file");
			return -1;
		}
	}

	memcpy(&pd->file->ops, ops, sizeof(struct osdp_file_ops));
	file_state_reset(pd->file);
	return 0;
}

OSDP_EXPORT
int osdp_get_file_tx_status(const osdp_t *ctx, int pd_idx,
			    int *size, int *offset)
{
	input_check(ctx, pd_idx);
	struct osdp_file *f = TO_FILE(osdp_to_pd(ctx, pd_idx));

	if (f->state != OSDP_FILE_INPROG && f->state != OSDP_FILE_DONE) {
		LOG_PRINT("File TX not in progress");
		return -1;
	}

	*size = f->size;
	*offset = f->offset;
	return 0;
}