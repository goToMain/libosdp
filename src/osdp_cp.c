/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_common.h"

#define TAG "CP: "

#define CMD_POLL_LEN                   1
#define CMD_LSTAT_LEN                  1
#define CMD_ISTAT_LEN                  1
#define CMD_OSTAT_LEN                  1
#define CMD_RSTAT_LEN                  1
#define CMD_ID_LEN                     2
#define CMD_CAP_LEN                    2
#define CMD_DIAG_LEN                   2
#define CMD_OUT_LEN                    5
#define CMD_LED_LEN                    15
#define CMD_BUZ_LEN                    6
#define CMD_TEXT_LEN                   7   /* variable length command */
#define CMD_COMSET_LEN                 6
#define CMD_KEYSET_LEN                 19
#define CMD_CHLNG_LEN                  9
#define CMD_SCRYPT_LEN                 17

#define REPLY_ACK_DATA_LEN             0
#define REPLY_PDID_DATA_LEN            12
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_DATA_LEN          2
#define REPLY_RSTATR_DATA_LEN          1
#define REPLY_COM_DATA_LEN             5
#define REPLY_NAK_DATA_LEN             1
#define REPLY_CCRYPT_DATA_LEN          32
#define REPLY_RMAC_I_DATA_LEN          16
#define REPLY_KEYPPAD_DATA_LEN         2   /* variable length command */
#define REPLY_RAW_DATA_LEN             4   /* variable length command */
#define REPLY_FMT_DATA_LEN             3   /* variable length command */
#define REPLY_BUSY_DATA_LEN            0

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
static int cp_build_command(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	uint8_t *smb;
	struct osdp_cmd *cmd = NULL;
	int data_off, i, ret = -1, len = 0;

	data_off = osdp_phy_packet_get_data_offset(pd, buf);
	smb = osdp_phy_packet_get_smb(pd, buf);

	buf += data_off;
	max_len -= data_off;
	if (max_len <= 0) {
		return -1;
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
		if (max_len < CMD_POLL_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_LSTAT:
		if (max_len < CMD_LSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_ISTAT:
		if (max_len < CMD_ISTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_OSTAT:
		if (max_len < CMD_OSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_RSTAT:
		if (max_len < CMD_RSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_ID:
		if (max_len < CMD_ID_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_CAP:
		if (max_len < CMD_CAP_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_DIAG:
		if (max_len < CMD_DIAG_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_OUT:
		if (max_len < CMD_OUT_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.tmr_count);
		buf[len++] = BYTE_1(cmd->output.tmr_count);
		ret = 0;
		break;
	case CMD_LED:
		if (max_len < CMD_LED_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->led.reader;
		buf[len++] = cmd->led.led_number;

		buf[len++] = cmd->led.temporary.control_code;
		buf[len++] = cmd->led.temporary.on_count;
		buf[len++] = cmd->led.temporary.off_count;
		buf[len++] = cmd->led.temporary.on_color;
		buf[len++] = cmd->led.temporary.off_color;
		buf[len++] = BYTE_0(cmd->led.temporary.timer);
		buf[len++] = BYTE_1(cmd->led.temporary.timer);

		buf[len++] = cmd->led.permanent.control_code;
		buf[len++] = cmd->led.permanent.on_count;
		buf[len++] = cmd->led.permanent.off_count;
		buf[len++] = cmd->led.permanent.on_color;
		buf[len++] = cmd->led.permanent.off_color;
		ret = 0;
		break;
	case CMD_BUZ:
		if (max_len < CMD_BUZ_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.tone_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		ret = 0;
		break;
	case CMD_TEXT:
		cmd = (struct osdp_cmd *)pd->cmd_data;
		if (max_len < (CMD_TEXT_LEN + cmd->text.length)) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.cmd;
		buf[len++] = cmd->text.temp_time;
		buf[len++] = cmd->text.offset_row;
		buf[len++] = cmd->text.offset_col;
		buf[len++] = cmd->text.length;
		for (i = 0; i < cmd->text.length; i++) {
			buf[len++] = cmd->text.data[i];
		}
		ret = 0;
		break;
	case CMD_COMSET:
		if (max_len < CMD_COMSET_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->comset.addr;
		buf[len++] = BYTE_0(cmd->comset.baud);
		buf[len++] = BYTE_1(cmd->comset.baud);
		buf[len++] = BYTE_2(cmd->comset.baud);
		buf[len++] = BYTE_3(cmd->comset.baud);
		ret = 0;
		break;
	case CMD_KEYSET:
		if (!ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
			LOG_ERR(TAG "Cannot perform KEYSET without SC!");
			return -1;
		}
		if (max_len < CMD_KEYSET_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 1;  /* key type (1: SCBK) */
		buf[len++] = 16; /* key length in bytes */
		osdp_compute_scbk(pd, buf + len);
		len += 16;
		ret = 0;
		break;
	case CMD_CHLNG:
		if (smb == NULL || max_len < CMD_CHLNG_LEN) {
			break;
		}
		osdp_fill_random(pd->sc.cp_random, 8);
		smb[0] = 3;       /* length */
		smb[1] = SCS_11;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		for (i = 0; i < 8; i++)
			buf[len++] = pd->sc.cp_random[i];
		ret = 0;
		break;
	case CMD_SCRYPT:
		if (smb == NULL || max_len < CMD_SCRYPT_LEN) {
			break;
		}
		osdp_compute_cp_cryptogram(pd);
		smb[0] = 3;       /* length */
		smb[1] = SCS_13;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		for (i = 0; i < 16; i++)
			buf[len++] = pd->sc.cp_cryptogram[i];
		ret = 0;
		break;
	default:
		LOG_ERR(TAG "Unknown/Unsupported command! CMD: %02x", cmd->id);
		return -1;
	}

	if (smb && (smb[1] > SCS_14) && ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
		/**
		 * When SC active and current cmd is not a handshake (<= SCS_14)
		 * then we must set SCS type to 17 if this message has data
		 * bytes and 15 otherwise.
		 */
		smb[0] = 2;
		smb[1] = (len > 1) ? SCS_17 : SCS_15;
	}

	if (ret < 0) {
		LOG_ERR(TAG "Unable to build CMD: %02x", cmd->id);
		return -1;
	}

	return len;
}

/**
 * Returns:
 *  0: success
 * -1: error
 *  2: retry current command
 */
static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp *ctx = TO_CTX(pd);
	int i, ret = -1, pos = 0, t1, t2;

	if (len < 1) {
		LOG_ERR("response must have at least one byte");
		return -1;
	}

	pd->reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	switch (pd->reply_id) {
	case REPLY_ACK:
		if (len != REPLY_ACK_DATA_LEN) {
			break;
		}
		ret = 0;
		break;
	case REPLY_NAK:
		if (len != REPLY_NAK_DATA_LEN) {
			break;
		}
		LOG_ERR(TAG "PD replied with NAK code %d", buf[pos]);
		ret = 0;
		break;
	case REPLY_PDID:
		if (len != REPLY_PDID_DATA_LEN) {
			break;
		}
		pd->id.vendor_code  = buf[pos++];
		pd->id.vendor_code |= buf[pos++] << 8;
		pd->id.vendor_code |= buf[pos++] << 16;

		pd->id.model = buf[pos++];
		pd->id.version = buf[pos++];

		pd->id.serial_number  = buf[pos++];
		pd->id.serial_number |= buf[pos++] << 8;
		pd->id.serial_number |= buf[pos++] << 16;
		pd->id.serial_number |= buf[pos++] << 24;

		pd->id.firmware_version  = buf[pos++] << 16;
		pd->id.firmware_version |= buf[pos++] << 8;
		pd->id.firmware_version |= buf[pos++];
		ret = 0;
		break;
	case REPLY_PDCAP:
		if ((len % REPLY_PDCAP_ENTITY_LEN) != 0) {
			break;
		}
		while (pos < len) {
			int func_code = buf[pos++];
			if (func_code > OSDP_PD_CAP_SENTINEL)
				break;
			pd->cap[func_code].function_code    = func_code;
			pd->cap[func_code].compliance_level = buf[pos++];
			pd->cap[func_code].num_items        = buf[pos++];
		}
		/* post-capabilities hooks */
		if (pd->cap[OSDP_PD_CAP_COMMUNICATION_SECURITY].compliance_level & 0x01)
			SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
		else
			CLEAR_FLAG(pd, PD_FLAG_SC_CAPABLE);
		ret = 0;
		break;
	case REPLY_LSTATR:
		if (len != REPLY_LSTATR_DATA_LEN) {
			break;
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_TAMPER);
		}else {
			CLEAR_FLAG(pd, PD_FLAG_TAMPER);
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_POWER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_POWER);
		}
		ret = 0;
		break;
	case REPLY_RSTATR:
		if (len != REPLY_RSTATR_DATA_LEN) {
			break;
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_R_TAMPER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_R_TAMPER);
		}
		ret = 0;
		break;
	case REPLY_COM:
		if (len != REPLY_COM_DATA_LEN) {
			break;
		}
		t1 = buf[pos++];
		temp32  = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_CRIT(TAG "COMSET responded with ID:%d baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		SET_FLAG(pd, PD_FLAG_COMSET_INPROG);
		ret = 0;
		break;
	case REPLY_CCRYPT:
		if (len != REPLY_CCRYPT_DATA_LEN) {
			break;
		}
		for (i=0; i<8; i++) {
			pd->sc.pd_client_uid[i] = buf[pos++];
		}
		for (i=0; i<8; i++) {
			pd->sc.pd_random[i] = buf[pos++];
		}
		for (i=0; i<16; i++) {
			pd->sc.pd_cryptogram[i] = buf[pos++];
		}
		osdp_compute_session_keys(TO_CTX(pd));
		if (osdp_verify_pd_cryptogram(pd) != 0) {
			LOG_ERR(TAG "failed to verify PD_crypt");
			return -1;
		}
		ret = 0;
		break;
	case REPLY_RMAC_I:
		if (len != REPLY_RMAC_I_DATA_LEN) {
			break;
		}
		for (i = 0; i < 16; i++) {
			pd->sc.r_mac[i] = buf[pos++];
		}
		SET_FLAG(pd, PD_FLAG_SC_ACTIVE);
		ret = 0;
		break;
	case REPLY_KEYPPAD:
		if (len < REPLY_KEYPPAD_DATA_LEN) {
			break;
		}
		pos++;	         /* reader number; skip */
		t1 = buf[pos++]; /* key length */
		if ((len - REPLY_KEYPPAD_DATA_LEN) != t1) {
			break;
		}
		if (ctx->notifier.keypress) {
			for (i = 0; i < t1; i++) {
				t2 = buf[pos + i]; /* key data */
				ctx->notifier.keypress(pd->offset, t2);
			}
		}
		ret = 0;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN) {
			break;
		}
		pos++;	                /* reader number; skip */
		t1 = buf[pos++];        /* format */
		t2 = buf[pos++];        /* length LSB */
		t2 |= buf[pos++] << 8; /* length MSB */
		if ((len - REPLY_RAW_DATA_LEN) != t2) {
			break;
		}
		if (ctx->notifier.cardread) {
			ctx->notifier.cardread(pd->offset, t1, buf + pos, t2);
		}
		ret = 0;
		break;
	case REPLY_FMT:
		if (len < REPLY_FMT_DATA_LEN) {
			break;
		}
		pos++;	/* reader number; skip */
		pos++;	/* skip one byte -- TODO: handle reader direction */
		t1 = buf[pos++]; /* Key length */
		if ((len - REPLY_FMT_DATA_LEN) != t1) {
			break;
		}
		if (ctx->notifier.cardread) {
			ctx->notifier.cardread(pd->offset, OSDP_CARD_FMT_ASCII,
					       buf + pos, t1);
		}
		ret = 0;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		if (len != REPLY_BUSY_DATA_LEN) {
			break;
		}
		ret = 2;
		break;
	default:
		LOG_DBG(TAG "unexpected reply: 0x%02x", pd->reply_id);
		return -1;
	}

	if (ret == -1) {
		LOG_ERR(TAG "REPLY %02x for CMD %02x format error!",
			pd->cmd_id, pd->reply_id);
		return -1;
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "CMD: %02x REPLY: %02x", pd->cmd_id, pd->reply_id);
	}

	return ret;
}

/**
 * cp_send_command - blocking send; doesn't handle partials
 * Returns:
 *   0 - success
 *  -1 - failure
 */
static int cp_send_command(struct osdp_pd *pd)
{
	int ret, len;

	/* init packet buf with header */
	len = osdp_phy_packet_init(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (len < 0) {
		return -1;
	}

	/* fill command data */
	ret = cp_build_command(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (ret < 0) {
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, pd->rx_buf, len, sizeof(pd->rx_buf));
	if (len < 0) {
		return -1;
	}

	ret = pd->channel.send(pd->channel.data, pd->rx_buf, len);

#ifdef CONFIG_OSDP_PACKET_TRACE
	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "bytes sent"); /* to get PD context printed */
		osdp_dump(NULL, pd->rx_buf, len);
	}
#endif

	return (ret == len) ? 0 : -1;
}

/**
 * cp_process_reply - received buffer from serial stream handling partials
 * Returns:
 *  0: success
 * -1: error
 *  1: no data yet
 *  2: re-issue command
 */
static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int rec_bytes, ret, max_len;

	buf = pd->rx_buf + pd->rx_buf_len;
	max_len = sizeof(pd->rx_buf) - pd->rx_buf_len;

	rec_bytes = pd->channel.recv(pd->channel.data, buf, max_len);
	if (rec_bytes <= 0)	/* No data received */
		return 1;
	pd->rx_buf_len += rec_bytes;

#ifdef CONFIG_OSDP_PACKET_TRACE
	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "bytes received"); /* to get PD context printed */
		osdp_dump(NULL, pd->rx_buf, pd->rx_buf_len);
	}
#endif

	/* Valid OSDP packet in buffer */
	ret = osdp_phy_decode_packet(pd, pd->rx_buf, pd->rx_buf_len);
	if (ret == OSDP_ERR_PKT_FMT) {
		return -1; /* fatal errors */
	} else if (ret == OSDP_ERR_PKT_WAIT) {
		/* rx_buf_len != pkt->len; wait for more data */
		return 1;
	} else if (ret == OSDP_ERR_PKT_SKIP) {
		/* soft fail - discard this message */
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		return 1;
	}
	pd->rx_buf_len = ret;

	return cp_decode_response(pd, pd->rx_buf, pd->rx_buf_len);
}

static void cp_free_command(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	osdp_slab_free(pd->cmd_slab, cmd);
}

static int cp_alloc_command(struct osdp_pd *pd, struct osdp_cmd **cmd)
{
	void *p;

	p = osdp_slab_alloc(pd->cmd_slab);
	if (p == NULL) {
		LOG_WRN(TAG "Failed to alloc cmd");
		return -1;
	}
	*cmd = p;
	return 0;
}

static void cp_enqueue_command(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct osdp_cmd_queue *q = &pd->queue;

	cmd->__next = NULL;
	if (q->front == NULL) {
		q->front = q->back = cmd;
	} else {
		assert(q->back);
		q->back->__next = cmd;
		q->back = cmd;
	}
}

static int cp_dequeue_command(struct osdp_pd *pd, struct osdp_cmd **cmd)
{
	struct osdp_cmd_queue *q = &pd->queue;

	if (q->front == NULL)
		return -1;
	*cmd = q->front;
	q->front = q->front->__next;
	return 0;
}

static void cp_flush_command_queue(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	while (cp_dequeue_command(pd, &cmd) == 0) {
		cp_free_command(pd, cmd);
	}
}

static inline void cp_set_offline(struct osdp_pd *pd)
{
	pd->cp_state = OSDP_CP_STATE_OFFLINE;
	pd->tstamp = osdp_millis_now();
}

static inline void cp_reset_state(struct osdp_pd *pd)
{
	pd->cp_state = OSDP_CP_STATE_INIT;
	osdp_phy_state_reset(pd);
	pd->flags = 0;
}

static inline void cp_set_state(struct osdp_pd *pd, enum osdp_cp_state_e state)
{
	pd->cp_state = state;
	CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
}

/**
 * Returns:
 *  -1: phy is in error state. Main state machine must reset it.
 *   0: No command in queue
 *   1: Command is in progress; this method must to be called again later.
 *   2: In command boundaries. There may or may not be other commands in queue.
 *
 * Note: This method must not dequeue cmd unless it reaches an invalid state.
 */
static int cp_phy_state_update(struct osdp_pd *pd)
{
	int ret = 1, tmp;
	struct osdp_cmd *cmd = NULL;

	switch (pd->cp_phy_state) {
	case OSDP_CP_PHY_STATE_ERR_WAIT:
		ret = -1;
		break;
	case OSDP_CP_PHY_STATE_IDLE:
		if (cp_dequeue_command(pd, &cmd)) {
			ret = 0;
			break;
		}
		pd->cmd_id = cmd->id;
		memcpy(pd->cmd_data, cmd, sizeof(struct osdp_cmd));
		cp_free_command(pd, cmd);
		/* fall-thru */
	case OSDP_CP_PHY_STATE_SEND_CMD:
		if ((cp_send_command(pd)) < 0) {
			LOG_ERR(TAG "send command error");
			pd->cp_phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = -1;
			break;
		}
		pd->cp_phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		if ((tmp = cp_process_reply(pd)) == 0) {
			pd->cp_phy_state = OSDP_CP_PHY_STATE_CLEANUP;
			break;
		}
		if (tmp == 2) {
			LOG_INF(TAG "PD busy; retry last command");
			pd->phy_tstamp = osdp_millis_now();
			pd->cp_phy_state = OSDP_CP_PHY_STATE_WAIT;
			ret = 2;
			break;
		}
		if (tmp == -1) {
			pd->cp_phy_state = OSDP_CP_PHY_STATE_ERR;
			break;
		}
		if (osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			LOG_ERR(TAG "CMD: %02x - response timeout", pd->cmd_id);
			pd->cp_phy_state = OSDP_CP_PHY_STATE_ERR;
		}
		break;
	case OSDP_CP_PHY_STATE_WAIT:
		if (osdp_millis_since(pd->phy_tstamp) < OSDP_CMD_RETRY_WAIT_MS)
			break;
		pd->cp_phy_state = OSDP_CP_PHY_STATE_IDLE;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		cp_flush_command_queue(pd);
		pd->cp_phy_state = OSDP_CP_PHY_STATE_ERR_WAIT;
		ret = -1;
		break;
	case OSDP_CP_PHY_STATE_CLEANUP:
		pd->cp_phy_state = OSDP_CP_PHY_STATE_IDLE;
		ret = 2;
		break;
	}

	return ret;
}

/**
 * Returns:
 *   0: nothing done
 *   1: dispatched
 *  -1: error
 */
static int cp_cmd_dispatcher(struct osdp_pd *pd, int cmd)
{
	struct osdp_cmd *c;

	if (ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
		CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
		return 0;
	}

	if (cp_alloc_command(pd, &c)) {
		return -1;
	}

	c->id = cmd;
	cp_enqueue_command(pd, c);
	SET_FLAG(pd, PD_FLAG_AWAIT_RESP);
	return 1;
}

static int cp_state_update(struct osdp_pd *pd)
{
	int phy_state, soft_fail;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == 1 ||   /* commands are being executed */
	    phy_state == 2) {   /* in-between commands */
		return -1 * phy_state;
	}

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->cp_state == OSDP_CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->cp_state != OSDP_CP_STATE_OFFLINE &&
	    phy_state < 0 && soft_fail == 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successfull */

	switch (pd->cp_state) {
	case OSDP_CP_STATE_ONLINE:
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)  == false &&
		    ISSET_FLAG(pd, PD_FLAG_SC_CAPABLE) == true  &&
		    osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_SEC) {
			LOG_INF("retry SC after retry timeout");
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
		if (osdp_millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS) {
			break;
		}
		if (cp_cmd_dispatcher(pd, CMD_POLL) == 0) {
			pd->tstamp = osdp_millis_now();
		}
		break;
	case OSDP_CP_STATE_OFFLINE:
		if (osdp_millis_since(pd->tstamp) > OSDP_PD_ERR_RETRY_SEC) {
			cp_reset_state(pd);
		}
		break;
	case OSDP_CP_STATE_INIT:
		cp_set_state(pd, OSDP_CP_STATE_IDREQ);
		/* FALLTHRU */
	case OSDP_CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDID) {
			cp_set_offline(pd);
		}
		cp_set_state(pd, OSDP_CP_STATE_CAPDET);
		/* FALLTHRU */
	case OSDP_CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDCAP) {
			cp_set_offline(pd);
		}
		if (ISSET_FLAG(pd, PD_FLAG_SC_CAPABLE)) {
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_ONLINE);
		break;
	case OSDP_CP_STATE_SC_INIT:
		osdp_sc_init(pd);
		cp_set_state(pd, OSDP_CP_STATE_SC_CHLNG);
		/* FALLTHRU */
	case OSDP_CP_STATE_SC_CHLNG:
		if (cp_cmd_dispatcher(pd, CMD_CHLNG) != 0) {
			break;
		}
		if (phy_state < 0) {
			if (ISSET_FLAG(pd, PD_FLAG_SC_SCBKD_DONE)) {
				LOG_INF(TAG "SC Failed; online without SC");
				pd->sc_tstamp = osdp_millis_now();
				cp_set_state(pd, OSDP_CP_STATE_ONLINE);
				break;
			}
			SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			SET_FLAG(pd, PD_FLAG_SC_SCBKD_DONE);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			pd->cp_phy_state = 0; /* soft reset phy state */
			LOG_WRN(TAG "SC Failed; retry with SCBK-D");
			break;
		}
		if (pd->reply_id != REPLY_CCRYPT) {
			LOG_ERR(TAG "CHLNG failed. Online without SC");
			pd->sc_tstamp = osdp_millis_now();
			cp_set_state(pd, OSDP_CP_STATE_ONLINE);
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_SC_SCRYPT);
		/* FALLTHRU */
	case OSDP_CP_STATE_SC_SCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_RMAC_I) {
			LOG_ERR(TAG "SCRYPT failed. Online without SC");
			pd->sc_tstamp = osdp_millis_now();
			cp_set_state(pd, OSDP_CP_STATE_ONLINE);
			break;
		}
		if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_WRN(TAG "SC ACtive with SCBK-D; Set SCBK");
			cp_set_state(pd, OSDP_CP_STATE_SET_SCBK);
			break;
		}
		LOG_INF(TAG "SC ACtive");
		pd->sc_tstamp = osdp_millis_now();
		cp_set_state(pd, OSDP_CP_STATE_ONLINE);
		break;
	case OSDP_CP_STATE_SET_SCBK:
		if (cp_cmd_dispatcher(pd, CMD_KEYSET) != 0) {
			break;
		}
		if (pd->reply_id == REPLY_NAK) {
			LOG_WRN(TAG "Failed to set SCBK; continue with SCBK-D");
			cp_set_state(pd, OSDP_CP_STATE_ONLINE);
			break;
		}
		LOG_INF(TAG "SCBK set; restarting SC to verify new SCBK");
		CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
		CLEAR_FLAG(pd, PD_FLAG_SC_ACTIVE);
		cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
		pd->seq_number = -1;
		break;
	default:
		break;
	}

	return 0;
}

/* --- Exported Methods --- */

OSDP_EXPORT
osdp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key)
{
	int i;
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;

	assert(info);
	assert(num_pd > 0);

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp");
		goto error;
	}
	ctx->magic = 0xDEADBEAF;
	ctx->flags |= FLAG_CP_MODE;
	if (master_key != NULL) {
		memcpy(ctx->sc_master_key, master_key, 16);
	}

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_cp");
		goto error;
	}
	cp = TO_CP(ctx);
	cp->__parent = ctx;
	cp->num_pd = num_pd;

	ctx->pd = calloc(1, sizeof(struct osdp_pd) * num_pd);
	if (ctx->pd == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_pd[]");
		goto error;
	}

	for (i = 0; i < num_pd; i++) {
		osdp_pd_info_t *p = info + i;
		pd = TO_PD(ctx, i);
		pd->offset = i;
		pd->__parent = ctx;
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->flags;
		pd->seq_number = -1;
		pd->cmd_slab = osdp_slab_init(sizeof(struct osdp_cmd),
					      OSDP_CP_CMD_POOL_SIZE);
		if (pd->cmd_slab == NULL) {
			LOG_ERR(TAG "failed to alloc struct osdp_cp_cmd_slab");
			goto error;
		}
		memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));
	}
	SET_CURRENT_PD(ctx, 0);
	LOG_INF(TAG "setup complete");
	return (osdp_t *) ctx;

error:
	osdp_cp_teardown((osdp_t *) ctx);
	return NULL;
}

OSDP_EXPORT
void osdp_cp_teardown(osdp_t *ctx)
{
	int i;

	if (ctx != NULL) {
		return;
	}

	for (i = 0; i < TO_CP(ctx)->num_pd; i++) {
		osdp_slab_del(TO_PD(ctx, i)->cmd_slab);
	}
	safe_free(TO_PD(ctx, 0));
	safe_free(TO_CP(ctx));
	safe_free(ctx);
}

OSDP_EXPORT
void osdp_cp_refresh(osdp_t *ctx)
{
	int i;

	assert(ctx);

	for (i = 0; i < NUM_PD(ctx); i++) {
		SET_CURRENT_PD(ctx, i);
		osdp_log_ctx_set(i);
		cp_state_update(GET_CURRENT_PD(ctx));
	}
}

OSDP_EXPORT
int osdp_cp_set_callback_key_press(osdp_t *ctx,
	int (*cb) (int address, uint8_t key))
{
	assert(ctx);

	(TO_OSDP(ctx))->notifier.keypress = cb;

	return 0;
}

OSDP_EXPORT
int osdp_cp_set_callback_card_read(osdp_t *ctx,
	int (*cb) (int address, int format, uint8_t *data, int len))
{
	assert(ctx);

	(TO_OSDP(ctx))->notifier.cardread = cb;

	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_output(osdp_t *ctx, int pd, struct osdp_cmd_output *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(TO_PD(ctx, pd), &cmd)) {
		return -1;
	}

	cmd->id = CMD_OUT;
	memcpy(&cmd->output, p, sizeof(struct osdp_cmd_output));
	cp_enqueue_command(TO_PD(ctx, pd), cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_led(osdp_t *ctx, int pd, struct osdp_cmd_led *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(TO_PD(ctx, pd), &cmd)) {
		return -1;
	}

	cmd->id = CMD_LED;
	memcpy(&cmd->led, p, sizeof(struct osdp_cmd_led));
	cp_enqueue_command(TO_PD(ctx, pd), cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_buzzer(osdp_t *ctx, int pd, struct osdp_cmd_buzzer *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(TO_PD(ctx, pd), &cmd)) {
		return -1;
	}

	cmd->id = CMD_BUZ;
	memcpy(&cmd->buzzer, p, sizeof(struct osdp_cmd_buzzer));
	cp_enqueue_command(TO_PD(ctx, pd), cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_text(osdp_t *ctx, int pd, struct osdp_cmd_text *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(TO_PD(ctx, pd), &cmd)) {
		return -1;
	}

	cmd->id = CMD_TEXT;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_text));
	cp_enqueue_command(TO_PD(ctx, pd), cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_comset(osdp_t *ctx, int pd, struct osdp_cmd_comset *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(TO_PD(ctx, pd), &cmd)) {
		return -1;
	}

	cmd->id = CMD_COMSET;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_comset));
	cp_enqueue_command(TO_PD(ctx, pd), cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_send_cmd_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p)
{
	int i;
	struct osdp_cmd *cmd;
	struct osdp_pd *pd;

	assert(ctx);

	if (osdp_get_status_mask(ctx) != PD_MASK(ctx)) {
		LOG_WRN(TAG "CMD_KEYSET can be sent only when all PDs are "
		          "ONLINE and SC_ACTIVE.");
		return 1;
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = TO_PD(ctx, i);
		if (osdp_slab_blocks(pd->cmd_slab) < NUM_PD(ctx)) {
			LOG_WRN(TAG "Out of slab space for keyset operation");
		}
		if (cp_alloc_command(pd, &cmd)) {
			return -1;
		}
		cmd->id = CMD_KEYSET;
		memcpy(&cmd->keyset, p, sizeof(struct osdp_cmd_keyset));
		cp_enqueue_command(pd, cmd);
	}

	return 0;
}

#ifdef UNIT_TESTING

/**
 * Force export some private methods for testing.
 */
int (*test_cp_alloc_command)(struct osdp_pd *, struct osdp_cmd **) = cp_alloc_command;
void (*test_cp_enqueue_command)(struct osdp_pd *, struct osdp_cmd *) = cp_enqueue_command;
int (*test_cp_phy_state_update)(struct osdp_pd *) = cp_phy_state_update;
int (*test_cp_state_update)(struct osdp_pd *) = cp_state_update;

#endif /* UNIT_TESTING */
