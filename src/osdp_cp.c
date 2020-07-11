/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_common.h"

#define TAG "CP: "

#define cp_set_offline(p)					\
	do {							\
		p->state = CP_STATE_OFFLINE;			\
		p->tstamp = millis_now();			\
	} while (0)

#define cp_set_state(p, s)					\
	do {							\
		p->state = s;					\
		clear_flag(p, PD_FLAG_AWAIT_RESP);		\
	} while (0)

#define cp_reset_state(p)					\
	do {							\
		p->state = CP_STATE_INIT;			\
		phy_state_reset(p);				\
		p->flags = 0;					\
	} while (0)

enum cp_phy_state_e {
	CP_PHY_STATE_IDLE,
	CP_PHY_STATE_SEND_CMD,
	CP_PHY_STATE_REPLY_WAIT,
	CP_PHY_STATE_WAIT,
	CP_PHY_STATE_ERR,
	CP_PHY_STATE_CLEANUP,
};

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int cp_build_command(struct osdp_pd *p, struct osdp_cmd *cmd, uint8_t *pkt)
{
	int ret = -1, i, len = 0;
	uint8_t *buf = phy_packet_get_data(p, pkt);
	uint8_t *smb = phy_packet_get_smb(p, pkt);

	// LOG_D(TAG "Building command 0x%02x",cmd->id);

	switch (cmd->id) {
	case CMD_POLL:
	case CMD_LSTAT:
	case CMD_ISTAT:
	case CMD_OSTAT:
	case CMD_RSTAT:
		buf[len++] = cmd->id;
		ret = 0;
		break;
	case CMD_ID:
	case CMD_CAP:
	case CMD_DIAG:
		buf[len++] = cmd->id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_OUT:
		buf[len++] = cmd->id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = byte_0(cmd->output.tmr_count);
		buf[len++] = byte_1(cmd->output.tmr_count);
		ret = 0;
		break;
	case CMD_LED:
		buf[len++] = cmd->id;
		buf[len++] = cmd->led.reader;
		buf[len++] = cmd->led.led_number;

		buf[len++] = cmd->led.temporary.control_code;
		buf[len++] = cmd->led.temporary.on_count;
		buf[len++] = cmd->led.temporary.off_count;
		buf[len++] = cmd->led.temporary.on_color;
		buf[len++] = cmd->led.temporary.off_color;
		buf[len++] = byte_0(cmd->led.temporary.timer);
		buf[len++] = byte_1(cmd->led.temporary.timer);

		buf[len++] = cmd->led.permanent.control_code;
		buf[len++] = cmd->led.permanent.on_count;
		buf[len++] = cmd->led.permanent.off_count;
		buf[len++] = cmd->led.permanent.on_color;
		buf[len++] = cmd->led.permanent.off_color;
		ret = 0;
		break;
	case CMD_BUZ:
		buf[len++] = cmd->id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.tone_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		ret = 0;
		break;
	case CMD_TEXT:
		buf[len++] = cmd->id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.cmd;
		buf[len++] = cmd->text.temp_time;
		buf[len++] = cmd->text.offset_row;
		buf[len++] = cmd->text.offset_col;
		buf[len++] = cmd->text.length;
		for (i = 0; i < cmd->text.length; i++)
			buf[len++] = cmd->text.data[i];
		ret = 0;
		break;
	case CMD_COMSET:
		buf[len++] = cmd->id;
		buf[len++] = cmd->comset.addr;
		buf[len++] = byte_0(cmd->comset.baud);
		buf[len++] = byte_1(cmd->comset.baud);
		buf[len++] = byte_2(cmd->comset.baud);
		buf[len++] = byte_3(cmd->comset.baud);
		ret = 0;
		break;
	case CMD_KEYSET:
		if (!isset_flag(p, PD_FLAG_SC_ACTIVE))
			break;
		buf[len++] = cmd->id;
		buf[len++] = 1;
		buf[len++] = 16;
		osdp_compute_scbk(p, buf + len);
		len += 16;
		ret = 0;
		break;
	case CMD_CHLNG:
		if (smb == NULL)
			break;
		smb[0] = 3;
		smb[1] = SCS_11;  /* type */
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = cmd->id;
		osdp_fill_random(p->sc.cp_random, 8);
		for (i=0; i<8; i++)
			buf[len++] = p->sc.cp_random[i];
		ret = 0;
		break;
	case CMD_SCRYPT:
		if (smb == NULL)
			break;
		smb[0] = 3;
		smb[1] = SCS_13;  /* type */
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = cmd->id;
		osdp_compute_cp_cryptogram(p);
		for (i=0; i<16; i++)
			buf[len++] = p->sc.cp_cryptogram[i];
		ret = 0;
		break;
	default:
		LOG_E(TAG "command 0x%02x isn't supported", cmd->id);
		break;
	}

	p->cmd_id = cmd->id;
	if (smb && (smb[1] > SCS_14) && isset_flag(p, PD_FLAG_SC_ACTIVE)) {
		smb[0] = 2;
		smb[1] = (len > 1) ? SCS_17 : SCS_15;
	}

	if (ret < 0) {
		LOG_W(TAG "cmd 0x%02x format error! -- %d", cmd->id, ret);
		return ret;
	}

	return len;
}

/**
 * Returns:
 *  0: success
 * -1: error
 *  2: retry current command
 */
int cp_decode_response(struct osdp_pd *p, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp *ctx = to_ctx(p);
	int i, ret = -1, pos = 0, t1, t2;

	if (len < 1) {
		LOG_E("response must have at least one byte");
		return -1;
	}

	p->reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	switch (p->reply_id) {
	case REPLY_ACK:
		ret = 0;
		break;
	case REPLY_NAK:
		if (buf[pos]) {
			LOG_E(TAG "NAK: %s", get_nac_reason(buf[pos]));
		}
		ret = 0;
		break;
	case REPLY_PDID:
		if (len != 12) {
			LOG_D(TAG "PDID format error, %d/%d",
				 len, pos);
			break;
		}
		p->id.vendor_code  = buf[pos++];
		p->id.vendor_code |= buf[pos++] << 8;
		p->id.vendor_code |= buf[pos++] << 16;

		p->id.model = buf[pos++];
		p->id.version = buf[pos++];

		p->id.serial_number  = buf[pos++];
		p->id.serial_number |= buf[pos++] << 8;
		p->id.serial_number |= buf[pos++] << 16;
		p->id.serial_number |= buf[pos++] << 24;

		p->id.firmware_version  = buf[pos++] << 16;
		p->id.firmware_version |= buf[pos++] << 8;
		p->id.firmware_version |= buf[pos++];
		ret = 0;
		break;
	case REPLY_PDCAP:
		if (len % 3 != 0) {
			LOG_D(TAG "PDCAP format error, %d/%d", len, pos);
			break;
		}
		while (pos < len) {
			int func_code = buf[pos++];
			if (func_code > CAP_SENTINEL)
				break;
			p->cap[func_code].function_code    = func_code;
			p->cap[func_code].compliance_level = buf[pos++];
			p->cap[func_code].num_items        = buf[pos++];
		}
		/* post-capabilities hooks */
		if (p->cap[CAP_COMMUNICATION_SECURITY].compliance_level & 0x01)
			set_flag(p, PD_FLAG_SC_CAPABLE);
		else
			clear_flag(p, PD_FLAG_SC_CAPABLE);
		ret = 0;
		break;
	case REPLY_LSTATR:
		buf[pos++] ? set_flag(p, PD_FLAG_TAMPER) : clear_flag(p, PD_FLAG_TAMPER);
		buf[pos++] ? set_flag(p, PD_FLAG_POWER) : clear_flag(p, PD_FLAG_POWER);
		ret = 0;
		break;
	case REPLY_RSTATR:
		buf[pos++] ? set_flag(p, PD_FLAG_R_TAMPER) : clear_flag(p, PD_FLAG_R_TAMPER);
		ret = 0;
		break;
	case REPLY_COM:
		t1 = buf[pos++];
		temp32  = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_C(TAG "COMSET responded with ID:%d baud:%d", t1, temp32);
		p->baud_rate = temp32;
		set_flag(p, PD_FLAG_COMSET_INPROG);
		ret = 0;
		break;
	case REPLY_CCRYPT:
		for (i=0; i<8; i++)
			p->sc.pd_client_uid[i] = buf[pos++];
		for (i=0; i<8; i++)
			p->sc.pd_random[i] = buf[pos++];
		for (i=0; i<16; i++)
			p->sc.pd_cryptogram[i] = buf[pos++];
		osdp_compute_session_keys(to_ctx(p));
		if (osdp_verify_pd_cryptogram(p) != 0) {
			LOG_E(TAG "failed to verify PD_crypt");
			break;
		}
		ret = 0;
		break;
	case REPLY_RMAC_I:
		if (len != 16)
			break;
		for (i=0; i<16; i++)
			p->sc.r_mac[i] = buf[pos++];
		set_flag(p, PD_FLAG_SC_ACTIVE);
		ret = 0;
		break;
	case REPLY_KEYPPAD:
		pos++;	/* skip one byte */
		t1 = buf[pos++]; /* key length */
		if (ctx->notifier.keypress) {
			for (i = 0; i < t1; i++) {
				t2 = buf[pos + i]; /* key */
				ctx->notifier.keypress(p->offset, t2);
			}
		}
		ret = 0;
		break;
	case REPLY_RAW:
		pos++;	/* skip one byte */
		t1 = buf[pos++]; /* format */
		t2 = buf[pos++]; /* length */
		len |= buf[pos++] << 8;
		if (ctx->notifier.cardread) {
			ctx->notifier.cardread(p->offset, t1, buf + pos, t2);
		}
		ret = 0;
		break;
	case REPLY_FMT:
		pos++;	/* skip one byte */
		pos++;	/* skip one byte -- TODO: discuss about reader direction */
		t1 = buf[pos++]; /* Key length */
		if (ctx->notifier.cardread) {
			ctx->notifier.cardread(p->offset, OSDP_CARD_FMT_ASCII,
					       buf + pos, t1);
		}
		ret = 0;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		ret = 2;
		break;
	default:
		LOG_D(TAG "unexpected reply: 0x%02x", p->reply_id);
		break;
	}

	if (p->cmd_id != CMD_POLL) {
		LOG_D(TAG "OUT(CMD): 0x%02x -- IN(REPLY): 0x%02x[%d]",
		p->cmd_id, p->reply_id, len);
	}

	return ret;
}

/**
 * cp_send_command - blocking send; doesn't handle partials
 * Returns:
 *   0 - success
 *  -1 - failure
 */
int cp_send_command(struct osdp_pd *p, struct osdp_cmd *cmd)
{
	int ret, len;
	uint8_t buf[OSDP_PACKET_BUF_SIZE];

	if (cmd == NULL)
		return -1;

	len = phy_build_packet_head(p, cmd->id, buf, OSDP_PACKET_BUF_SIZE);
	/* init packet buf with header */
	if (len < 0) {
		LOG_E(TAG "failed at phy_build_packet_head");
		return -1;
	}

	/* fill command data */
	ret = cp_build_command(p, cmd, buf);
	if (ret < 0) {
		LOG_E(TAG "failed to build command 0x%02x", cmd->id);
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = phy_build_packet_tail(p, buf, len, OSDP_PACKET_BUF_SIZE);
	if (len < 0) {
		LOG_E(TAG "failed to build command 0x%02x", cmd->id);
		return -1;
	}

#ifdef OSDP_PACKET_TRACE
	if (p->cmd_id != CMD_POLL) {
		LOG_EM(TAG "bytes sent");
		osdp_dump(NULL, buf, len);
	}
#endif

	ret = p->channel.send(p->channel.data, buf, len);

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
int cp_process_reply(struct osdp_pd *p)
{
	int ret;

	ret = p->channel.recv(p->channel.data, p->phy_rx_buf + p->phy_rx_buf_len,
			   OSDP_PACKET_BUF_SIZE - p->phy_rx_buf_len);
	if (ret <= 0)	/* No data received */
		return 1;
	p->phy_rx_buf_len += ret;

#ifdef OSDP_PACKET_TRACE
	if (p->cmd_id != CMD_POLL) {
		LOG_EM(TAG "bytes received");
		osdp_dump(NULL, p->phy_rx_buf, p->phy_rx_buf_len);
	}
#endif

	/* Valid OSDP packet in buffer */

	ret = phy_decode_packet(p, p->phy_rx_buf, p->phy_rx_buf_len);
	switch(ret) {
	case -1: /* fatal errors */
		LOG_E(TAG "failed to decode packet");
		return -1;
	case -2: /* rx_buf_len != pkt->len; wait for more data */
		return 1;
	case -3: /* soft fail */
	case -4: /* rx_buf had invalid MARK or SOM */
		/* Reset rx_buf_len so next call can start afresh */
		p->phy_rx_buf_len = 0;
		return 1;
	}

	return cp_decode_response(p, p->phy_rx_buf, ret);
}

inline void cp_free_command(struct osdp *ctx, struct osdp_cmd *cmd)
{
	osdp_slab_free(ctx->cmd_slab, cmd);
}

int cp_alloc_command(struct osdp *ctx, struct osdp_cmd **cmd)
{
	void *p;

	p = osdp_slab_alloc(ctx->cmd_slab);
	if (p == NULL) {
		LOG_W(TAG "Failed to alloc cmd");
		return -1;
	}
	*cmd = p;
	return 0;
}

void cp_enqueue_command(struct osdp_pd *p, struct osdp_cmd *cmd)
{
	struct osdp_cmd_queue *q = &p->queue;

	cmd->__next = NULL;
	if (q->front == NULL) {
		q->front = q->back = cmd;
	} else {
		assert(q->back);
		q->back->__next = cmd;
		q->back = cmd;
	}
}

int cp_dequeue_command(struct osdp_pd *p, struct osdp_cmd **cmd, int read_only)
{
	struct osdp_cmd_queue *q = &p->queue;

	if (q->front == NULL)
		return -1;
	*cmd = q->front;
	if (!read_only)
		q->front = q->front->__next;
	return 0;
}

void cp_flush_command_queue(struct osdp_pd *p)
{
	struct osdp_cmd *cmd;

	while (cp_dequeue_command(p, &cmd, FALSE) == 0) {
		cp_free_command(to_ctx(p), cmd);
	}
}

void cp_flush_all_cmd_queues(struct osdp *ctx)
{
	int i;
	struct osdp_pd *pd;

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = to_pd(ctx, i);
		cp_flush_command_queue(pd);
	}
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
int cp_phy_state_update(struct osdp_pd *pd)
{
	int ret = 0;
	struct osdp_cmd *cmd = NULL;

	switch (pd->phy_state) {
	case CP_PHY_STATE_IDLE:
		if (cp_dequeue_command(pd, &cmd, TRUE))
			break;
		/* fall-thru */
	case CP_PHY_STATE_SEND_CMD:
		if ((cp_send_command(pd, cmd)) < 0) {
			LOG_I(TAG "command dispatch error");
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
			break;
		}
		pd->phy_state = CP_PHY_STATE_REPLY_WAIT;
		pd->phy_rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = millis_now();
		ret = 1;
		break;
	case CP_PHY_STATE_REPLY_WAIT:
		if ((ret = cp_process_reply(pd)) == 0) {
			pd->phy_state = CP_PHY_STATE_CLEANUP;
			ret = 2;
			break;
		}
		if (ret == 2) {
			LOG_I(TAG "PD busy; retry last command");
			pd->phy_tstamp = millis_now();
			pd->phy_state = CP_PHY_STATE_WAIT;
			break;
		}
		if (ret == -1) {
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
			break;
		}
		if (millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			LOG_I(TAG "CMD: 0x%02x - response timeout", pd->cmd_id);
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
		}
		break;
	case CP_PHY_STATE_WAIT:
		ret = 1;
		if (millis_since(pd->phy_tstamp) < OSDP_CP_RETRY_WAIT_MS)
			break;
		pd->phy_state = CP_PHY_STATE_IDLE;
		break;
	case CP_PHY_STATE_ERR:
		cp_flush_command_queue(pd);
		ret = -1;
		break;
	case CP_PHY_STATE_CLEANUP:
		if (cp_dequeue_command(pd, &cmd, FALSE) < 0) {
			// Got to cleanup but pop failed?
			LOG_EM(TAG "command pop failed!");
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
			break;
		}
		cp_free_command(to_ctx(pd), cmd);
		pd->phy_state = CP_PHY_STATE_IDLE;
		ret = 2;
		break;
	}

	return ret;
}

int cp_cmd_dispatcher(struct osdp_pd *p, int cmd)
{
	struct osdp_cmd *c;

	if (isset_flag(p, PD_FLAG_AWAIT_RESP)) {
		clear_flag(p, PD_FLAG_AWAIT_RESP);
		return 0;
	}

	if (cp_alloc_command(to_ctx(p), &c))
		return -1;

	c->id = cmd;
	cp_enqueue_command(p, c);
	set_flag(p, PD_FLAG_AWAIT_RESP);
	return 1;
}

int cp_state_update(struct osdp_pd *pd)
{
	int phy_state, soft_fail;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == 1 ||	/* commands are being executed */
	    phy_state == 2)	/* in-between commands */
		return -1 * phy_state;

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->state == CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->state != CP_STATE_OFFLINE && phy_state < 0 && soft_fail == 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successfull */

	switch (pd->state) {
	case CP_STATE_ONLINE:
		if (isset_flag(pd, PD_FLAG_SC_ACTIVE)  == FALSE &&
		    isset_flag(pd, PD_FLAG_SC_CAPABLE) == TRUE  &&
		    millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_SEC) {
			LOG_I("retry SC after retry timeout");
			cp_set_state(pd, CP_STATE_SC_INIT);
			break;
		}
		if (millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS)
			break;
		if (cp_cmd_dispatcher(pd, CMD_POLL) == 0)
			pd->tstamp = millis_now();
		break;
	case CP_STATE_OFFLINE:
		if (millis_since(pd->tstamp) > OSDP_PD_ERR_RETRY_SEC)
			cp_reset_state(pd);
		break;
	case CP_STATE_INIT:
		cp_set_state(pd, CP_STATE_IDREQ);
		/* FALLTHRU */
	case CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0)
			break;
		if (pd->reply_id != REPLY_PDID)
			cp_set_offline(pd);
		cp_set_state(pd, CP_STATE_CAPDET);
		/* FALLTHRU */
	case CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0)
			break;
		if (pd->reply_id != REPLY_PDCAP)
			cp_set_offline(pd);
		if (isset_flag(pd, PD_FLAG_SC_CAPABLE)) {
			cp_set_state(pd, CP_STATE_SC_INIT);
			break;
		}
		cp_set_state(pd, CP_STATE_ONLINE);
		break;
	case CP_STATE_SC_INIT:
		osdp_sc_init(pd);
		cp_set_state(pd, CP_STATE_SC_CHLNG);
		/* FALLTHRU */
	case CP_STATE_SC_CHLNG:
		if (cp_cmd_dispatcher(pd, CMD_CHLNG) != 0)
			break;
		if (phy_state < 0) {
			if (isset_flag(pd, PD_FLAG_SC_SCBKD_DONE)) {
				LOG_I(TAG "SC Failed; set to online without SC");
				pd->sc_tstamp = millis_now();
				cp_set_state(pd, CP_STATE_ONLINE);
				break;
			}
			set_flag(pd, PD_FLAG_SC_USE_SCBKD);
			set_flag(pd, PD_FLAG_SC_SCBKD_DONE);
			cp_set_state(pd, CP_STATE_SC_INIT);
			pd->phy_state = 0; /* soft reset phy state */
			LOG_W(TAG "SC Failed; retry with SCBK-D");
			break;
		}
		if (pd->reply_id != REPLY_CCRYPT) {
			LOG_E(TAG "CHLNG failed. Online without SC");
			pd->sc_tstamp = millis_now();
			cp_set_state(pd, CP_STATE_ONLINE);
			break;
		}
		cp_set_state(pd, CP_STATE_SC_SCRYPT);
		/* FALLTHRU */
	case CP_STATE_SC_SCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0)
			break;
		if (pd->reply_id != REPLY_RMAC_I) {
			LOG_E(TAG "SCRYPT failed. Online without SC");
			pd->sc_tstamp = millis_now();
			cp_set_state(pd, CP_STATE_ONLINE);
			break;
		}
		if (isset_flag(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_W(TAG "SC ACtive with SCBK-D; Set SCBK");
			cp_set_state(pd, CP_STATE_SET_SCBK);
			break;
		}
		LOG_I(TAG "SC ACtive");
		pd->sc_tstamp = millis_now();
		cp_set_state(pd, CP_STATE_ONLINE);
		break;
	case CP_STATE_SET_SCBK:
		if (cp_cmd_dispatcher(pd, CMD_KEYSET) != 0)
			break;
		if (pd->reply_id == REPLY_NAK) {
			LOG_W(TAG "Failed to set SCBK; continue with SCBK-D");
			cp_set_state(pd, CP_STATE_ONLINE);
			break;
		}
		LOG_I(TAG "SCBK set; Restarting SC setup");
		clear_flag(pd, PD_FLAG_SC_USE_SCBKD);
		clear_flag(pd, PD_FLAG_SC_ACTIVE);
		cp_set_state(pd, CP_STATE_SC_INIT);
		break;
	default:
		break;
	}

	return 0;
}

/* --- Exported Methods --- */

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
		LOG_E(TAG "failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;
	ctx->flags |= FLAG_CP_MODE;
	if (master_key != NULL)
		memcpy(ctx->sc_master_key, master_key, 16);

	ctx->cmd_slab = osdp_slab_init(sizeof(struct osdp_cmd),
				       OSDP_CP_CMD_POOL_SIZE * num_pd);
	if (ctx->cmd_slab == NULL) {
		LOG_E(TAG "failed to alloc struct osdp_cp_cmd_slab");
		goto malloc_err;
	}

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		LOG_E(TAG "failed to alloc struct osdp_cp");
		goto malloc_err;
	}
	cp = to_cp(ctx);
	node_set_parent(cp, ctx);
	cp->num_pd = num_pd;

	ctx->pd = calloc(1, sizeof(struct osdp_pd) * num_pd);
	if (ctx->pd == NULL) {
		LOG_E(TAG "failed to alloc struct osdp_pd[]");
		goto malloc_err;
	}

	for (i = 0; i < num_pd; i++) {
		osdp_pd_info_t *p = info + i;
		pd = to_pd(ctx, i);
		pd->offset = i;
		node_set_parent(pd, ctx);
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->flags;
		pd->seq_number = -1;
		memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));
	}
	set_current_pd(ctx, 0);
	LOG_I(TAG "setup complete");
	return (osdp_t *) ctx;

 malloc_err:
	osdp_cp_teardown((osdp_t *) ctx);
	return NULL;
}

void osdp_cp_teardown(osdp_t *ctx)
{
	if (ctx != NULL) {
		safe_free(to_pd(ctx, 0));
		osdp_slab_del(to_osdp(ctx)->cmd_slab);
		safe_free(to_cp(ctx));
		safe_free(ctx);
	}
}

void osdp_cp_refresh(osdp_t *ctx)
{
	int i;

	assert(ctx);

	for (i = 0; i < NUM_PD(ctx); i++) {
		set_current_pd(ctx, i);
		osdp_log_ctx_set(i);
		cp_state_update(to_current_pd(ctx));
	}
}

int osdp_cp_set_callback_key_press(osdp_t *ctx,
	int (*cb) (int address, uint8_t key))
{
	assert(ctx);

	(to_osdp(ctx))->notifier.keypress = cb;

	return 0;
}

int osdp_cp_set_callback_card_read(osdp_t *ctx,
	int (*cb) (int address, int format, uint8_t *data, int len))
{
	assert(ctx);

	(to_osdp(ctx))->notifier.cardread = cb;

	return 0;
}

int osdp_cp_send_cmd_output(osdp_t *ctx, int pd, struct osdp_cmd_output *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_E(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(to_osdp(ctx), &cmd))
		return -1;

	cmd->id = CMD_OUT;
	memcpy(&cmd->output, p, sizeof(struct osdp_cmd_output));
	cp_enqueue_command(to_pd(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_led(osdp_t *ctx, int pd, struct osdp_cmd_led *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_E(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(to_osdp(ctx), &cmd))
		return -1;

	cmd->id = CMD_LED;
	memcpy(&cmd->led, p, sizeof(struct osdp_cmd_led));
	cp_enqueue_command(to_pd(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_buzzer(osdp_t *ctx, int pd, struct osdp_cmd_buzzer *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_E(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(to_osdp(ctx), &cmd))
		return -1;

	cmd->id = CMD_BUZ;
	memcpy(&cmd->buzzer, p, sizeof(struct osdp_cmd_buzzer));
	cp_enqueue_command(to_pd(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_text(osdp_t *ctx, int pd, struct osdp_cmd_text *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_E(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(to_osdp(ctx), &cmd))
		return -1;

	cmd->id = CMD_TEXT;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_text));
	cp_enqueue_command(to_pd(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_comset(osdp_t *ctx, int pd, struct osdp_cmd_comset *p)
{
	struct osdp_cmd *cmd;

	assert(ctx);
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_E(TAG "Invalid PD number");
		return -1;
	}

	if (cp_alloc_command(to_osdp(ctx), &cmd))
		return -1;

	cmd->id = CMD_COMSET;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_comset));
	cp_enqueue_command(to_pd(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p)
{
	int i;
	struct osdp_cmd *cmd;
	struct osdp_pd *pd;

	assert(ctx);

	if (osdp_get_status_mask(ctx) != PD_MASK(ctx)) {
		LOG_W(TAG "CMD_KEYSET can be sent only when all PDs are "
		          "ONLINE and SC_ACTIVE.");
		return 1;
	}

	if (osdp_slab_blocks(to_osdp(ctx)->cmd_slab) < NUM_PD(ctx)) {
		LOG_W(TAG "Out of slab space for keyset operation");
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = to_pd(ctx, i);
		if (cp_alloc_command(to_osdp(ctx), &cmd))
			return -1;
		cmd->id = CMD_KEYSET;
		memcpy(&cmd->keyset, p, sizeof(struct osdp_cmd_keyset));
		cp_enqueue_command(pd, cmd);
	}

	return 0;
}
