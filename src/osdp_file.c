/*
 * Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "osdp_file.h"

#define FILE_TRANSFER_HEADER_SIZE     11
#define FILE_TRANSFER_STAT_SIZE       7

#define OSDP_FILE_TX_STATUS_ACK                0
#define OSDP_FILE_TX_STATUS_CONTENTS_PROCESSED 1
#define OSDP_FILE_TX_STATUS_PD_RESET           2
#define OSDP_FILE_TX_STATUS_KEEP_ALIVE         3
#define OSDP_FILE_TX_STATUS_ERR_ABORT         -1
#define OSDP_FILE_TX_STATUS_ERR_UNKNOWN       -2
#define OSDP_FILE_TX_STATUS_ERR_INVALID       -3

#define OSDP_FILE_TX_FLAG_EXCLUSIVE            0x01000000
#define OSDP_FILE_TX_FLAG_PLAIN_TEXT           0x02000000
#define OSDP_FILE_TX_FLAG_POLL_RESP            0x04000000

static inline void file_state_reset(struct osdp_file *f)
{
	f->flags = 0;
	f->offset = 0;
	f->length = 0;
	f->errors = 0;
	f->size = 0;
	f->state = OSDP_FILE_IDLE;
	f->file_id = 0;
	f->tstamp = 0;
	f->wait_time_ms = 0;
	f->cancel_req = false;
}

static inline bool file_tx_in_progress(struct osdp_file *f)
{
	return f && f->state == OSDP_FILE_INPROG;
}

/* --- Sender CMD/RESP Handers --- */

static void write_file_tx_header(struct osdp_file *f, uint8_t *buf)
{
	int len = 0;

	U8_TO_BYTES_LE(f->file_id, buf, len);
	U32_TO_BYTES_LE(f->size, buf, len);
	U32_TO_BYTES_LE(f->offset, buf, len);
	U16_TO_BYTES_LE(f->length, buf, len);
	assert(len == FILE_TRANSFER_HEADER_SIZE);
}

int osdp_file_cmd_tx_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int buf_available;
	struct osdp_file *f = TO_FILE(pd);
	uint8_t *data = buf + FILE_TRANSFER_HEADER_SIZE;

	/**
	 * We should never reach this function if a valid file transfer as in
	 * progress.
	 */
	BUG_ON(f == NULL);
	BUG_ON(f->state != OSDP_FILE_INPROG && f->state != OSDP_FILE_KEEP_ALIVE);

	if ((size_t)max_len <= FILE_TRANSFER_HEADER_SIZE) {
		LOG_ERR("TX_Build: insufficient space; need:%zu have:%d",
			FILE_TRANSFER_HEADER_SIZE, max_len);
		goto reply_abort;
	}

	if (ISSET_FLAG(f, OSDP_FILE_TX_FLAG_PLAIN_TEXT)) {
		LOG_WRN("TX_Build: Ignoring plaintext file transfer request");
	}

	if (f->state == OSDP_FILE_KEEP_ALIVE) {
		LOG_DBG("TX_Build: keep-alive");
		write_file_tx_header(f, buf);
		return FILE_TRANSFER_HEADER_SIZE;
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
	buf_available = max_len - FILE_TRANSFER_HEADER_SIZE - 16;

	f->length = f->ops.read(f->ops.arg, data, buf_available, f->offset);
	if (f->length < 0) {
		LOG_ERR("TX_Build: user read failed! rc:%d len:%d off:%d",
			f->length, buf_available, f->offset);
		goto reply_abort;
	}
	if (f->length == 0) {
		LOG_WRN("TX_Build: Read 0 length chunk");
		goto reply_abort;
	}

	/* fill the packet buffer (layout: struct osdp_cmd_file_xfer) */
	write_file_tx_header(f, buf);

	return FILE_TRANSFER_HEADER_SIZE + f->length;

reply_abort:
	LOG_ERR("TX_Build: Aborting file transfer due to unrecoverable error!");
	file_state_reset(f);
	return -1;
}

int osdp_file_cmd_stat_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int pos = 0;
	bool do_close = false;
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_stat stat;

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

	/* Collect struct osdp_cmd_file_stat */
	BYTES_TO_U8_LE(buf, pos, stat.control);
	BYTES_TO_U16_LE(buf, pos, stat.delay);
	BYTES_TO_U16_LE(buf, pos, stat.status);
	BYTES_TO_U16_LE(buf, pos, stat.rx_size);
	assert(pos == len);
	assert(f->offset + f->length <= f->size);

	/* Collect control flags */
	SET_FLAG_V(f, OSDP_FILE_TX_FLAG_EXCLUSIVE, !(stat.control & 0x01))
	SET_FLAG_V(f, OSDP_FILE_TX_FLAG_PLAIN_TEXT, stat.control & 0x02)
	SET_FLAG_V(f, OSDP_FILE_TX_FLAG_POLL_RESP, stat.control & 0x04)

	f->offset += f->length;
	do_close = f->length && (f->offset == f->size);
	f->wait_time_ms = stat.delay;
	f->tstamp = osdp_millis_now();
	f->length = 0;
	f->errors = 0;

	if (f->offset != f->size) {
		/* Transfer is in progress */
		return 0;
	}

	/* File transfer complete; close file and end file transfer */

	if (do_close && f->ops.close(f->ops.arg) < 0) {
		LOG_ERR("Stat_Decode: Close failed! ... continuing");
	}

	switch (stat.status) {
	case OSDP_FILE_TX_STATUS_KEEP_ALIVE:
		f->state = OSDP_FILE_KEEP_ALIVE;
		LOG_INF("Stat_Decode: File transfer done; keep alive");
		return 0;
	case OSDP_FILE_TX_STATUS_PD_RESET:
		make_request(pd, CP_REQ_OFFLINE);
		__fallthrough;
	case OSDP_FILE_TX_STATUS_CONTENTS_PROCESSED:
		f->state = OSDP_FILE_DONE;
		LOG_INF("Stat_Decode: File transfer complete");
		return 0;
	default:
		LOG_ERR("Stat_Decode: File transfer error; "
		        "status:%d offset:%d", stat.status, f->offset);
		f->errors++;
		return -1;
	}
}

/* --- Receiver CMD/RESP Handler --- */

int osdp_file_cmd_tx_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int rc;
	int pos = 0;
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_xfer xfer;
	struct osdp_cmd cmd;
	uint8_t *data = buf + FILE_TRANSFER_HEADER_SIZE;

	if (f == NULL) {
		LOG_ERR("TX_Decode: File ops not registered!");
		return -1;
	}

	if ((size_t)len <= sizeof(struct osdp_cmd_file_xfer)) {
		LOG_ERR("TX_Decode: invalid decode len:%d exp>=%zu",
			len, sizeof(struct osdp_cmd_file_xfer));
		return -1;
	}

	BYTES_TO_U8_LE(buf, pos, xfer.type);
	BYTES_TO_U32_LE(buf, pos, xfer.size);
	BYTES_TO_U32_LE(buf, pos, xfer.offset);
	BYTES_TO_U16_LE(buf, pos, xfer.length);
	assert(pos == sizeof(struct osdp_cmd_file_xfer));
	assert(xfer.length + pos == len);

	if (f->state == OSDP_FILE_IDLE || f->state == OSDP_FILE_DONE) {
		if (pd->command_callback) {
			/**
			 * Notify app of this command and make sure
			 * we can proceed
			 */
			cmd.id = OSDP_CMD_FILE_TX;
			cmd.file_tx.flags = f->flags;
			cmd.file_tx.id = xfer.type;
			rc = pd->command_callback(pd->command_callback_arg, &cmd);
			if (rc < 0)
				return -1;
		}

		/* new file write request */
		int size = (int)xfer.size;
		if (f->ops.open(f->ops.arg, xfer.type, &size) < 0) {
			LOG_ERR("TX_Decode: Open failed! fd:%d", xfer.type);
			return -1;
		}

		LOG_INF("TX_Decode: Starting file transfer of size: %d", xfer.size);
		file_state_reset(f);
		f->file_id = xfer.type;
		f->size = xfer.size;
		f->state = OSDP_FILE_INPROG;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("TX_Decode: File transfer is not in progress!");
		return -1;
	}

	f->length = f->ops.write(f->ops.arg, data, xfer.length, xfer.offset);
	if (f->length != xfer.length) {
		LOG_ERR("TX_Decode: user write failed! rc:%d len:%d off:%d",
			f->length, xfer.length, xfer.offset);
		f->errors++;
		return -1;
	}

	return 0;
}

int osdp_file_cmd_stat_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int len = 0;
	struct osdp_file *f = TO_FILE(pd);
	struct osdp_cmd_file_stat stat = {
		.status = OSDP_FILE_TX_STATUS_ACK,
		.control = 0x01, /* interleaving, secure channel, no activity */
	};

	if (f == NULL) {
		LOG_ERR("Stat_Build: File ops not registered!");
		return -1;
	}

	if (f->state != OSDP_FILE_INPROG) {
		LOG_ERR("Stat_Build: File transfer is not in progress!");
		return -1;
	}

	if ((size_t)max_len < sizeof(struct osdp_cmd_file_stat)) {
		LOG_ERR("Stat_Build: insufficient space; need:%zu have:%d",
			sizeof(struct osdp_cmd_file_stat), max_len);
		return -1;
	}

	if (f->length > 0) {
		f->offset += f->length;
	} else {
		stat.status = OSDP_FILE_TX_STATUS_ERR_INVALID;
	}
	LOG_DBG("length: %d offset: %d size: %d", f->length, f->offset, f->size);
	f->length = 0;
	assert(f->offset <= f->size);
	if (f->offset == f->size) { /* EOF */
		if (f->ops.close(f->ops.arg) < 0) {
			LOG_ERR("Stat_Build: Close failed!");
			return -1;
		}
		f->state = OSDP_FILE_DONE;
		stat.status = OSDP_FILE_TX_STATUS_CONTENTS_PROCESSED;
		LOG_INF("TX_Decode: File receive complete");
	}

	/* fill the packet buffer (layout: struct osdp_cmd_file_stat) */

	U8_TO_BYTES_LE(stat.control, buf, len);
	U16_TO_BYTES_LE(stat.delay, buf, len);
	U16_TO_BYTES_LE(stat.status, buf, len);
	U16_TO_BYTES_LE(stat.rx_size, buf, len);
	assert(len == FILE_TRANSFER_STAT_SIZE);

	return len;
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

/**
 * @brief Return the next command that the CP should send to the PD.
 *
 * @param pd PD context
 * @retval +ve - Send this OSDP command
 * @retval  -1 - don't send any command, wait for me
 * @retval   0 - nothing to send; let some other module decide
 */
int osdp_file_tx_get_command(struct osdp_pd *pd)
{
	struct osdp_file *f = TO_FILE(pd);

	if (!f || f->state == OSDP_FILE_IDLE || f->state == OSDP_FILE_DONE) {
		return 0;
	}

	if (f->errors > OSDP_FILE_ERROR_RETRY_MAX || f->cancel_req) {
		LOG_ERR("Aborting transfer of file fd:%d", f->file_id);
		osdp_file_tx_abort(pd);
		return CMD_ABORT;
	}

	if (f->wait_time_ms &&
	    osdp_millis_since(f->tstamp) < f->wait_time_ms) {
		return ISSET_FLAG(f, OSDP_FILE_TX_FLAG_EXCLUSIVE) ? -1 : 0;
	}

	if (ISSET_FLAG(f, OSDP_FILE_TX_FLAG_POLL_RESP)) {
		return CMD_POLL;
	}

	return CMD_FILETRANSFER;
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