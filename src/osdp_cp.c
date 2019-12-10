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
};

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int cp_build_command(struct osdp_pd *p, struct osdp_data *cmd, uint8_t *pkt)
{
	union cmd_all *c;
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
		if (cmd->len != sizeof(struct osdp_data) + 4)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = cmd->id;
		buf[len++] = c->output.output_no;
		buf[len++] = c->output.control_code;
		buf[len++] = byte_0(c->output.tmr_count);
		buf[len++] = byte_1(c->output.tmr_count);
		ret = 0;
		break;
	case CMD_LED:
		if (cmd->len != sizeof(struct osdp_data) + 16)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = cmd->id;
		buf[len++] = c->led.reader;
		buf[len++] = c->led.number;

		buf[len++] = c->led.temporary.control_code;
		buf[len++] = c->led.temporary.on_count;
		buf[len++] = c->led.temporary.off_count;
		buf[len++] = c->led.temporary.on_color;
		buf[len++] = c->led.temporary.off_color;
		buf[len++] = byte_0(c->led.temporary.timer);
		buf[len++] = byte_1(c->led.temporary.timer);

		buf[len++] = c->led.permanent.control_code;
		buf[len++] = c->led.permanent.on_count;
		buf[len++] = c->led.permanent.off_count;
		buf[len++] = c->led.permanent.on_color;
		buf[len++] = c->led.permanent.off_color;
		ret = 0;
		break;
	case CMD_BUZ:
		if (cmd->len != sizeof(struct osdp_data) + 5)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = cmd->id;
		buf[len++] = c->buzzer.reader;
		buf[len++] = c->buzzer.tone_code;
		buf[len++] = c->buzzer.on_count;
		buf[len++] = c->buzzer.off_count;
		buf[len++] = c->buzzer.rep_count;
		ret = 0;
		break;
	case CMD_TEXT:
		if (cmd->len != sizeof(struct osdp_data) + 38)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = cmd->id;
		buf[len++] = c->text.reader;
		buf[len++] = c->text.cmd;
		buf[len++] = c->text.temp_time;
		buf[len++] = c->text.offset_row;
		buf[len++] = c->text.offset_col;
		buf[len++] = c->text.length;
		for (i = 0; i < c->text.length; i++)
			buf[len++] = c->text.data[i];
		ret = 0;
		break;
	case CMD_COMSET:
		if (cmd->len != sizeof(struct osdp_data) + 5)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = cmd->id;
		buf[len++] = c->comset.addr;
		buf[len++] = byte_0(c->comset.baud);
		buf[len++] = byte_1(c->comset.baud);
		buf[len++] = byte_2(c->comset.baud);
		buf[len++] = byte_3(c->comset.baud);
		ret = 0;
		break;
	case CMD_KEYSET:
		if (cmd->len != sizeof(struct osdp_data) + 18)
			break;
		c = (union cmd_all *)cmd->data;
		buf[len++] = c->keyset.key_type;
		buf[len++] = c->keyset.len;
		for (i=0; i<c->keyset.len; i++)
			buf[len++] = c->keyset.data[i];
		break;
	case CMD_CHLNG:
		if (smb == NULL)
			break;
		smb[1] = SCS_11;  /* type */
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 1 : 0;
		buf[len++] = cmd->id;
		osdp_fill_random(p->sc.cp_random, 8);
		for (i=0; i<8; i++)
			buf[len++] = p->sc.cp_random[i];
		ret = 0;
		break;
	case CMD_SCRYPT:
		if (smb == NULL)
			break;
		smb[1] = SCS_13;  /* type */
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 1 : 0;
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

	if (smb && isset_flag(p, PD_FLAG_SC_ACTIVE))
		smb[1] = (len > 1) ? SCS_17 : SCS_15;

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
	int i, ret = -1, reply_id, pos = 0, t1, t2;

	reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	// LOG_D("Processing resp 0x%02x with %d data bytes", reply_id, len);

	switch (reply_id) {
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
				ctx->notifier.keypress(p->address, t2);
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
			ctx->notifier.cardread(p->address, t1, buf + pos, t2);
		}
		ret = 0;
		break;
	case REPLY_FMT:
		pos++;	/* skip one byte */
		pos++;	/* skip one byte -- TODO: discuss about reader direction */
		t1 = buf[pos++]; /* Key length */
		if (ctx->notifier.cardread) {
			ctx->notifier.cardread(p->address, OSDP_CARD_FMT_ASCII,
					       buf + pos, t1);
		}
		ret = 0;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		ret = 2;
		break;
	default:
		LOG_D(TAG "unexpected reply: 0x%02x", reply_id);
		break;
	}

	return ret;
}

/**
 * cp_send_command - blocking send; doesn't handle partials
 * Returns:
 *   0 - success
 *  -1 - failure
 */
int cp_send_command(struct osdp_pd *p, struct osdp_data *cmd)
{
	int ret, len;
	uint8_t buf[OSDP_PACKET_BUF_SIZE];

	len = phy_build_packet_head(p, cmd->id, buf, OSDP_PACKET_BUF_SIZE);
	/* init packet buf with header */
	if (len < 0) {
		LOG_E(TAG "failed at phy_build_packet_head");
		return -1;
	}

	/* fill command data */
	ret = cp_build_command(p, cmd, buf);
	if (ret < 0) {
		LOG_E(TAG "failed to build command %d", cmd->id);
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = phy_build_packet_tail(p, buf, len, OSDP_PACKET_BUF_SIZE);
	if (len < 0) {
		LOG_E(TAG "failed to build command %d", cmd->id);
		return -1;
	}

#ifdef OSDP_PACKET_TRACE
	osdp_dump("CP_SEND:", buf, len);
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

	ret = phy_check_packet(p->phy_rx_buf, p->phy_rx_buf_len);
	if (ret < 0) {	/* rx_buf had invalid MARK or SOM. Reset buf_len */
		p->phy_rx_buf_len = 0;
		return 1;
	}
	if (ret > 1)	/* rx_buf_len != pkt->len; wait for more data */
		return 1;

	/* Valid OSDP packet in buffer */

#ifdef OSDP_PACKET_TRACE
	osdp_dump("CP_RECV:", p->phy_rx_buf, p->phy_rx_buf_len);
#endif

	ret = phy_decode_packet(p, p->phy_rx_buf, p->phy_rx_buf_len);
	if (ret < 0) {
		LOG_E(TAG "failed decode response");
		return -1;
	}
	p->phy_rx_buf_len = ret;

	ret = cp_decode_response(p, p->phy_rx_buf, p->phy_rx_buf_len);
	p->phy_rx_buf_len = 0; /* reset buf_len for next use */

	return ret;
}

int cp_enqueue_command(struct osdp_pd *p, struct osdp_data *c)
{
	int len, fs, start, end;
	struct cmd_queue *q = p->queue;

	fs = q->tail - q->head;
	if (fs <= 0)
		fs += OSDP_PD_CMD_QUEUE_SIZE;

	len = c->len;
	if (len > fs)
		return -1;

	start = q->head + 1;
	if (start >= OSDP_PD_CMD_QUEUE_SIZE)
		start = 0;

	if (start == q->tail)
		return -1;

	end = start + len;
	if (end >= OSDP_PD_CMD_QUEUE_SIZE)
		end = end % OSDP_PD_CMD_QUEUE_SIZE;

	if (start > end) {
		memcpy(q->buffer + start, c, OSDP_PD_CMD_QUEUE_SIZE - start);
		memcpy(q->buffer,
		       (uint8_t *) c + OSDP_PD_CMD_QUEUE_SIZE - start, end);
	} else {
		memcpy(q->buffer + start, c, len);
	}

	q->head = end;
	return 0;
}

void cp_flush_command_queue(struct osdp_pd *pd)
{
	pd->queue->head = 0;
	pd->queue->head = 0;
}

void cp_flush_all_cmd_queues(struct osdp *ctx)
{
	int i;
	struct osdp_pd *pd;

	for (i = 0; i < ctx->cp->num_pd; i++) {
		pd = to_pd(ctx, i);
		cp_flush_command_queue(pd);
	}
}

int cp_dequeue_command(struct osdp_pd *pd, int readonly, uint8_t * cmd_buf, int maxlen)
{
	int start, end, len;
	struct cmd_queue *q = pd->queue;

	if (q->head == q->tail)
		return 0;	/* empty */

	start = q->tail + 1;
	len = q->buffer[start];

	if (len > maxlen)
		return -1;

	end = start + len;
	if (end >= OSDP_PD_CMD_QUEUE_SIZE)
		end = end % OSDP_PD_CMD_QUEUE_SIZE;

	if (start > end) {
		memcpy(cmd_buf, q->buffer + start, OSDP_PD_CMD_QUEUE_SIZE - start);
		memcpy(cmd_buf + OSDP_PD_CMD_QUEUE_SIZE - start, q->buffer, end);
	} else {
		memcpy(cmd_buf, q->buffer + start, len);
	}

	if (!readonly)
		q->tail = end;
	return len;
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
	uint8_t phy_cmd[OSDP_DATA_BUF_SIZE];
	struct osdp_data *cmd = (struct osdp_data *)phy_cmd;

	switch (pd->phy_state) {
	case CP_PHY_STATE_IDLE:
		ret = cp_dequeue_command(pd, FALSE, phy_cmd, OSDP_DATA_BUF_SIZE);
		if (ret == 0)
			break;
		if (ret < 0) {
			LOG_I(TAG "command dequeue error");
			cp_flush_command_queue(pd);
			pd->phy_state = CP_PHY_STATE_ERR;
			break;
		}
		ret = 1;
		/* fall-thru */
	case CP_PHY_STATE_SEND_CMD:
		if ((cp_send_command(pd, cmd)) < 0) {
			LOG_I(TAG "command dispatch error");
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
			break;
		}
		pd->phy_state = CP_PHY_STATE_REPLY_WAIT;
		pd->phy_tstamp = millis_now();
		break;
	case CP_PHY_STATE_REPLY_WAIT:
		if ((ret = cp_process_reply(pd)) == 0) {
			pd->phy_state = CP_PHY_STATE_IDLE;
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
			LOG_I(TAG "read response timeout");
			pd->phy_state = CP_PHY_STATE_ERR;
			ret = -1;
		}
		break;
	case CP_PHY_STATE_WAIT:
		ret = 1;
		if (millis_since(pd->phy_tstamp) < OSDP_CP_RETRY_WAIT_MS)
			break;
		pd->phy_state = CP_PHY_STATE_SEND_CMD;
		break;
	case CP_PHY_STATE_ERR:
		ret = -1;
		break;
	}

	return ret;
}

int cp_cmd_dispatcher(struct osdp_pd *p, int cmd)
{
	struct osdp_data c;

	if (isset_flag(p, PD_FLAG_AWAIT_RESP)) {
		clear_flag(p, PD_FLAG_AWAIT_RESP);
		return 0;
	}

	c.id = cmd;
	c.len = sizeof(struct osdp_data);
	if (cp_enqueue_command(p, &c) != 0) {
		LOG_W(TAG "command_enqueue error!");
	}
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
	if (phy_state < 0 && soft_fail == 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successfull */

	switch (pd->state) {
	case CP_STATE_ONLINE:
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
		cp_set_state(pd, CP_STATE_CAPDET);
		/* FALLTHRU */
	case CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0)
			break;
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
			set_flag(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, CP_STATE_SC_INIT);
			pd->phy_state = 0; /* soft reset phy state */
			LOG_W(TAG "SC Failed with SCBK; reting with SCBK-D");
			break;
		}
		cp_set_state(pd, CP_STATE_SC_SCRYPT);
		/* FALLTHRU */
	case CP_STATE_SC_SCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0)
			break;
		if (isset_flag(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_W(TAG "SC ACtive with SCBK-D");
		}
		cp_set_state(pd, CP_STATE_ONLINE);
		break;
	default:
		break;
	}

	return 0;
}

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key)
{
	int i;
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;

	if (num_pd <= 0 || info == NULL)
		return NULL;

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_E(TAG "failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;
	if (master_key != NULL)
		memcpy(ctx->sc_master_key, master_key, 16);

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
		pd->queue = calloc(1, sizeof(struct cmd_queue));
		if (pd->queue == NULL) {
			LOG_E(TAG "failed to alloc pd->cmd_queue");
			goto malloc_err;
		}
		node_set_parent(pd, ctx);
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->flags;
		pd->seq_number = -1;
		memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));
	}
	set_current_pd(ctx, 0);
	LOG_I(TAG "setup complete");
	return (osdp_cp_t *) ctx;

 malloc_err:
	osdp_cp_teardown((osdp_cp_t *) ctx);
	return NULL;
}

void osdp_cp_teardown(osdp_cp_t *ctx)
{
	int i;
	struct osdp_cp *cp;
	struct osdp_pd *pd;

	if (ctx == NULL)
		return;

	cp = to_cp(ctx);
	if (cp == NULL)
		return;

	for (i = cp->num_pd - 1; i >= 0; i--) {
		pd = to_pd(ctx, i);
		if (pd != NULL) {
			if (pd->queue != NULL)
				free(pd->queue);
			if (i == 0)
				free(pd);	// final
		}
	}

	free(cp);
	free(ctx);
}

void osdp_cp_refresh(osdp_cp_t *ctx)
{
	int i;

	for (i = 0; i < to_cp(ctx)->num_pd; i++) {
		set_current_pd(ctx, i);
		osdp_log_ctx_set(to_current_pd(ctx)->address);
		cp_state_update(to_current_pd(ctx));
		osdp_log_ctx_reset();
	}
}

int osdp_cp_set_callback_key_press(osdp_cp_t * ctx,
	int (*cb) (int address, uint8_t key))
{
	(to_osdp(ctx))->notifier.keypress = cb;

	return 0;
}

int osdp_cp_set_callback_card_read(osdp_cp_t * ctx,
	int (*cb) (int address, int format, uint8_t * data, int len))
{
	(to_osdp(ctx))->notifier.cardread = cb;

	return 0;
}

int osdp_cp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_OUT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_output);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_output));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_OUT enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_OUT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_led);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_led));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_OUT enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_BUZ;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_buzzer);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_buzzer));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_set_text(osdp_cp_t *ctx, int pd, struct osdp_cmd_text *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_TEXT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_text);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_text));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_COMSET;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_comset);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_comset));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_keyset(osdp_cp_t *ctx, struct osdp_cmd_keyset *p)
{
	int i;
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;
	struct osdp_cp *cp = to_osdp(ctx)->cp;
	struct osdp_pd *pd;

	for (i = 0; i < cp->num_pd; i++) {
		pd = to_pd(ctx, i);
		if (pd->state != CP_STATE_ONLINE ||
		    isset_flag(pd, PD_FLAG_SC_ACTIVE) == 0)
			break;
	}
	if (i < cp->num_pd) {
		LOG_W(TAG "CMD_KEYSET can be sent only when all PDs are "
		          "ONLINE and SC_ACTIVE.");
		return 1;
	}

	cmd->id = CMD_KEYSET;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_keyset);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_keyset));

	for (i = 0; i < cp->num_pd; i++) {
		pd = to_pd(ctx, i);
		if (cp_enqueue_command(pd, cmd) != 0) {
			LOG_EM(TAG "CMD_KEYSET enqueue error! Flushing all "
			           "commands in queue for all PDs.");
			cp_flush_all_cmd_queues(to_osdp(ctx));
			return -1;
		}
	}
	return 0;
}
