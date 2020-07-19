/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_common.h"

#define TAG "PD: "

void pd_enqueue_command(struct osdp_pd *p, struct osdp_cmd *cmd)
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

void pd_decode_command(struct osdp_pd *p, struct osdp_cmd *reply, uint8_t *buf,
		       int len)
{
	int i, ret = -1, pos = 0;
	struct osdp_cmd *cmd;

	reply->id = 0;
	p->cmd_id = buf[pos++];
	len--;

	switch (p->cmd_id) {
	case CMD_POLL:
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_LSTAT:
		reply->id = REPLY_LSTATR;
		ret = 0;
		break;
	case CMD_ISTAT:
		reply->id = REPLY_ISTATR;
		ret = 0;
		break;
	case CMD_OSTAT:
		reply->id = REPLY_OSTATR;
		ret = 0;
		break;
	case CMD_RSTAT:
		reply->id = REPLY_RSTATR;
		ret = 0;
		break;
	case CMD_ID:
		pos++;		// Skip reply type info.
		reply->id = REPLY_PDID;
		ret = 0;
		break;
	case CMD_CAP:
		pos++;		// Skip reply type info.
		reply->id = REPLY_PDCAP;
		ret = 0;
		break;
	case CMD_OUT:
		if (len != 4)
			break;
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_OUTPUT;
		cmd->output.output_no    = buf[pos++];
		cmd->output.control_code = buf[pos++];
		cmd->output.tmr_count    = buf[pos++];
		cmd->output.tmr_count   |= buf[pos++] << 8;
		pd_enqueue_command(p, cmd);
		reply->id = REPLY_OSTATR;
		ret = 0;
		break;
	case CMD_LED:
		if (len != 14)
			break;
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_LED;
		cmd->led.reader = buf[pos++];
		cmd->led.led_number = buf[pos++];

		cmd->led.temporary.control_code = buf[pos++];
		cmd->led.temporary.on_count     = buf[pos++];
		cmd->led.temporary.off_count    = buf[pos++];
		cmd->led.temporary.on_color     = buf[pos++];
		cmd->led.temporary.off_color    = buf[pos++];
		cmd->led.temporary.timer        = buf[pos++];
		cmd->led.temporary.timer       |= buf[pos++] << 8;

		cmd->led.permanent.control_code = buf[pos++];
		cmd->led.permanent.on_count     = buf[pos++];
		cmd->led.permanent.off_count    = buf[pos++];
		cmd->led.permanent.on_color     = buf[pos++];
		cmd->led.permanent.off_color    = buf[pos++];
		pd_enqueue_command(p, cmd);
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_BUZ:
		if (len != 5)
			break;
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_BUZZER;
		cmd->buzzer.reader    = buf[pos++];
		cmd->buzzer.tone_code = buf[pos++];
		cmd->buzzer.on_count  = buf[pos++];
		cmd->buzzer.off_count = buf[pos++];
		cmd->buzzer.rep_count = buf[pos++];
		pd_enqueue_command(p, cmd);
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_TEXT:
		if (len < 7)
			break;
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_TEXT;
		cmd->text.reader     = buf[pos++];
		cmd->text.cmd        = buf[pos++];
		cmd->text.temp_time  = buf[pos++];
		cmd->text.offset_row = buf[pos++];
		cmd->text.offset_col = buf[pos++];
		cmd->text.length     = buf[pos++];
		if (cmd->text.length > 32)
			break;
		for (i = 0; i < cmd->text.length; i++)
			cmd->text.data[i] = buf[pos++];
		pd_enqueue_command(p, cmd);
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_COMSET:
		if (len != 5)
			break;
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_COMSET;
		cmd->comset.addr  = buf[pos++];
		cmd->comset.baud  = buf[pos++];
		cmd->comset.baud |= buf[pos++] << 8;
		cmd->comset.baud |= buf[pos++] << 16;
		cmd->comset.baud |= buf[pos++] << 24;
		pd_enqueue_command(p, cmd);
		reply->id = REPLY_COM;
		ret = 0;
		break;
	case CMD_KEYSET:
		if (len != 18) {
			LOG_ERR(TAG "CMD_KEYSET length mismatch! %d/18", len);
			break;
		}
		/**
		 * For CMD_KEYSET to be accepted, PD must be
		 * ONLINE and SC_ACTIVE.
		 */
		if (isset_flag(p, PD_FLAG_SC_ACTIVE) == 0) {
			reply->id = REPLY_NAK;
			reply->cmd_bytes[0] = OSDP_PD_NAK_SC_COND;
			LOG_ERR(TAG "Keyset with SC inactive");
			break;
		}
		/* only key_type == 1 (SCBK) and key_len == 16 is supported */
		if (buf[pos] != 1 || buf[pos + 1] != 16) {
			LOG_ERR(TAG "Keyset invalid len/type: %d/%d",
			      buf[pos], buf[pos + 1]);
			break;
		}
		cmd = osdp_slab_alloc(to_ctx(p)->cmd_slab);
		if (cmd == NULL) {
			LOG_ERR(TAG "cmd alloc error");
			break;
		}
		cmd->id = OSDP_CMD_KEYSET;
		cmd->keyset.key_type = buf[pos++];
		cmd->keyset.len = buf[pos++];
		memcpy(cmd->keyset.data, buf + pos, 16);
		memcpy(p->sc.scbk, buf + pos, 16);
		pd_enqueue_command(p, cmd);
		clear_flag(p, PD_FLAG_SC_USE_SCBKD);
		clear_flag(p, PD_FLAG_INSTALL_MODE);
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_CHLNG:
		if (p->cap[CAP_COMMUNICATION_SECURITY].compliance_level == 0) {
			reply->id = REPLY_NAK;
			reply->cmd_bytes[0] = OSDP_PD_NAK_SC_UNSUP;
			break;
		}
		if (len != 8) {
			LOG_ERR(TAG "CMD_CHLNG length mismatch! %d/8", len);
			break;
		}
		osdp_sc_init(p);
		clear_flag(p, PD_FLAG_SC_ACTIVE);
		for (i = 0; i < 8; i++)
			p->sc.cp_random[i] = buf[pos++];
		reply->id = REPLY_CCRYPT;
		ret = 0;
		break;
	case CMD_SCRYPT:
		if (len != 16) {
			LOG_ERR(TAG "CMD_SCRYPT length mismatch! %d/16", len);
			break;
		}
		for (i = 0; i < 16; i++)
			p->sc.cp_cryptogram[i] = buf[pos++];
		reply->id = REPLY_RMAC_I;
		ret = 0;
		break;
	default:
		break;
	}

	if (ret != 0 && reply->id == 0) {
		reply->id = REPLY_NAK;
		reply->cmd_bytes[0] = OSDP_PD_NAK_RECORD;
	}

	p->reply_id = reply->id;
	if (p->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "IN(CMD): 0x%02x[%d] -- OUT(REPLY): 0x%02x",
		      p->cmd_id, len, p->reply_id);
	}
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int pd_build_reply(struct osdp_pd *p, struct osdp_cmd *reply, uint8_t *buf,
		   int max_len)
{
	uint8_t *smb;
	int i, data_off, len = 0;

	data_off = phy_packet_get_data_offset(p, buf);
	smb = phy_packet_get_smb(p, buf);

	buf += data_off;
	max_len -= (data_off + 1); /* +1 for reply->id */
	if (max_len < 0) {
		LOG_ERR(TAG "Out of buffer space!");
		return -1;
	}
	buf[len++] = reply->id;

	switch (reply->id) {
	case REPLY_ACK:
		break;
	case REPLY_PDID:
		if (max_len < 12) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		buf[len++] = byte_0(p->id.vendor_code);
		buf[len++] = byte_1(p->id.vendor_code);
		buf[len++] = byte_2(p->id.vendor_code);

		buf[len++] = p->id.model;
		buf[len++] = p->id.version;

		buf[len++] = byte_0(p->id.serial_number);
		buf[len++] = byte_1(p->id.serial_number);
		buf[len++] = byte_2(p->id.serial_number);
		buf[len++] = byte_3(p->id.serial_number);

		buf[len++] = byte_3(p->id.firmware_version);
		buf[len++] = byte_2(p->id.firmware_version);
		buf[len++] = byte_1(p->id.firmware_version);
		break;
	case REPLY_PDCAP:
		for (i = 0; i < CAP_SENTINEL; i++) {
			if (p->cap[i].function_code != i)
				continue;
			if (max_len < 3) {
				LOG_ERR(TAG "Out of buffer space!");
				return -1;
			}
			buf[len++] = i;
			buf[len++] = p->cap[i].compliance_level;
			buf[len++] = p->cap[i].num_items;
			max_len -= 3;
		}
		break;
	case REPLY_LSTATR:
		if (max_len < 2) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		buf[len++] = isset_flag(p, PD_FLAG_TAMPER);
		buf[len++] = isset_flag(p, PD_FLAG_POWER);
		break;
	case REPLY_RSTATR:
		if (max_len < 1) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		buf[len++] = isset_flag(p, PD_FLAG_R_TAMPER);
		break;
	case REPLY_COM:
		if (max_len < 5) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		buf[len++] = p->address;
		buf[len++] = byte_0(p->baud_rate);
		buf[len++] = byte_1(p->baud_rate);
		buf[len++] = byte_2(p->baud_rate);
		buf[len++] = byte_3(p->baud_rate);
		break;
	case REPLY_NAK:
		if (max_len < 1) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		buf[len++] = reply->cmd_bytes[0];
		break;
	case REPLY_CCRYPT:
		if (smb == NULL)
			break;
		if (max_len < 32) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		osdp_fill_random(p->sc.pd_random, 8);
		osdp_compute_session_keys(to_ctx(p));
		osdp_compute_pd_cryptogram(p);
		for (i = 0; i < 8; i++)
			buf[len++] = p->sc.pd_client_uid[i];
		for (i = 0; i < 8; i++)
			buf[len++] = p->sc.pd_random[i];
		for (i = 0; i < 16; i++)
			buf[len++] = p->sc.pd_cryptogram[i];
		smb[0] = 3;
		smb[1] = SCS_12;
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		break;
	case REPLY_RMAC_I:
		if (smb == NULL)
			break;
		if (max_len < 16) {
			LOG_ERR(TAG "Out of buffer space!");
			return -1;
		}
		osdp_compute_rmac_i(p);
		for (i = 0; i < 16; i++)
			buf[len++] = p->sc.r_mac[i];
		smb[0] = 3;
		smb[1] = SCS_14;
		if (osdp_verify_cp_cryptogram(p) == 0)
			smb[2] = 0x01;
		else
			smb[2] = 0x00;
		set_flag(p, PD_FLAG_SC_ACTIVE);
		if (isset_flag(p, PD_FLAG_SC_USE_SCBKD))
			LOG_WRN(TAG "SC Active with SCBK-D");
		else
			LOG_INF(TAG "SC Active");
		break;
	}

	if (smb && (smb[1] > SCS_14) && isset_flag(p, PD_FLAG_SC_ACTIVE)) {
		smb[0] = 2;
		smb[1] = (len > 1) ? SCS_18 : SCS_16;
	}

	if (len == 0) {
		buf[len++] = REPLY_NAK;
		buf[len++] = OSDP_PD_NAK_SC_UNSUP;
	}

	return len;
}

/**
 * pd_send_reply - blocking send; doesn't handle partials
 * Returns:
 *   0 - success
 *  -1 - failure
 */
int pd_send_reply(struct osdp_pd *p, struct osdp_cmd *reply)
{
	int ret, len;

	/* init packet buf with header */
	len = phy_build_packet_head(p, reply->id, p->rx_buf, sizeof(p->rx_buf));
	if (len < 0) {
		LOG_ERR(TAG "Failed to build head; ID: %02X", reply->id);
		return -1;
	}

	/* fill reply data */
	ret = pd_build_reply(p, reply, p->rx_buf, sizeof(p->rx_buf));
	if (ret <= 0) {
		LOG_ERR(TAG "Failed build reply; ID: %02X", reply->id);
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = phy_build_packet_tail(p,  p->rx_buf, len, sizeof(p->rx_buf));
	if (len < 0) {
		LOG_ERR(TAG "Failed to finalize reply; ID: %02X", reply->id);
		return -1;
	}

	ret = p->channel.send(p->channel.data, p->rx_buf, len);

#ifdef CONFIG_OSDP_PACKET_TRACE
	if (p->cmd_id != CMD_POLL) {
		osdp_dump("PD sent", p->rx_buf, len);
	}
#endif

	return (ret == len) ? 0 : -1;
}

/**
 * pd_receve_packet - received buffer from serial stream handling partials
 * Returns:
 *  0: success
 * -1: error
 *  1: no data yet
 */
int pd_receve_packet(struct osdp_pd *pd)
{
	uint8_t *buf;
	int ret, new_packet, max_len;

	new_packet = pd->rx_buf_len == 0;
	buf = pd->rx_buf + pd->rx_buf_len;
	max_len = sizeof(pd->rx_buf) - pd->rx_buf_len;

	ret = pd->channel.recv(pd->channel.data, buf, max_len);
	if (ret <= 0)
		return 1;
	if (new_packet && ret > 0)
		pd->tstamp = millis_now();
	pd->rx_buf_len += ret;

#ifdef CONFIG_OSDP_PACKET_TRACE
	if (pd->rx_buf[6] != CMD_POLL && pd->rx_buf[8] != CMD_POLL) {
		osdp_dump("PD received", pd->rx_buf, pd->rx_buf_len);
	}
#endif

	ret = phy_decode_packet(pd, pd->rx_buf, pd->rx_buf_len);
	if (ret < 0) {
		switch(ret) {
		case -1: /* fatal error */
			LOG_ERR(TAG "failed to decode packet");
			pd->rx_buf_len = 0;
			if (pd->channel.flush)
				pd->channel.flush(pd->channel.data);
			ret = -1;
			break;
		case -2: /* rx_buf_len != pkt->len; wait for more data */
			ret = 1;
			break;
		case -3: /* soft fail - discard this message */
		case -4: /* malformed OSDP packet - reset rx_buf_len */
			pd->rx_buf_len = 0;
			if (pd->channel.flush)
				pd->channel.flush(pd->channel.data);
			ret = 1;
			break;
		default:
			ret = -1;
		}
		return ret;
	}
	pd->rx_buf_len = ret;
	return 0;
}

void pd_phy_state_update(struct osdp_pd *pd)
{
	int ret;
	struct osdp_cmd reply;

	switch (pd->phy_state) {
	case PD_PHY_STATE_IDLE:
		ret = pd_receve_packet(pd);
		if (ret < 0 || (pd->rx_buf_len > 0 &&
		    millis_since(pd->tstamp) > OSDP_RESP_TOUT_MS)) {
			/**
			 * When we receive a command from PD after a timeout,
			 * any established secure channel must be discarded.
			 */
			pd->phy_state = PD_PHY_STATE_ERR;
			break;
		}
		if (ret == 1)
			break;
		pd_decode_command(pd, &reply, pd->rx_buf, pd->rx_buf_len);
		pd->phy_state = PD_PHY_STATE_SEND_REPLY;
		/* FALLTHRU */
	case PD_PHY_STATE_SEND_REPLY:
		if (pd_send_reply(pd, &reply) == -1) {
			pd->phy_state = PD_PHY_STATE_ERR;
			break;
		}
		pd->rx_buf_len = 0;
		pd->phy_state = PD_PHY_STATE_IDLE;
		break;
	case PD_PHY_STATE_ERR:
		/**
		 * PD error state is momentary as it doesn't maintain any state
		 * between commands. We just clean up secure channel status and
		 * go back to idle state with a call to phy_state_reset().
		 */
		clear_flag(pd, PD_FLAG_SC_ACTIVE);
		pd->rx_buf_len = 0;
		pd->phy_state = PD_PHY_STATE_IDLE;
		break;
	}
}

void osdp_pd_set_attributes(struct osdp_pd *pd, struct pd_cap *cap,
			    struct pd_id *id)
{
	int fc;

	while (cap && ((fc = cap->function_code) > 0)) {
		if (fc >= CAP_SENTINEL)
			break;
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}

	memcpy(&pd->id, id, sizeof(struct pd_id));
}

/* --- Exported Methods --- */

osdp_t *osdp_pd_setup(osdp_pd_info_t *p, uint8_t *scbk)
{
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;

	assert(p);

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;

	ctx->cmd_slab = osdp_slab_init(sizeof(struct osdp_cmd),
				      OSDP_CP_CMD_POOL_SIZE);
	if (ctx->cmd_slab == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_cp_cmd_slab");
		goto malloc_err;
	}

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_cp");
		goto malloc_err;
	}
	cp = to_cp(ctx);
	node_set_parent(cp, ctx);
	cp->num_pd = 1;

	ctx->pd = calloc(1, sizeof(struct osdp_pd));
	if (ctx->pd == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_pd");
		goto malloc_err;
	}
	set_current_pd(ctx, 0);
	pd = to_pd(ctx, 0);

	node_set_parent(pd, ctx);
	pd->offset = 0;
	pd->baud_rate = p->baud_rate;
	pd->address = p->address;
	pd->flags = p->flags;
	pd->seq_number = -1;
	memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));

	if (scbk == NULL) {
		LOG_WRN(TAG "SCBK not provided. PD is in INSTALL_MODE");
		set_flag(pd, PD_FLAG_INSTALL_MODE);
	}
	else {
		memcpy(pd->sc.scbk, scbk, 16);
	}

	osdp_pd_set_attributes(pd, p->cap, &p->id);

	set_flag(pd, PD_FLAG_PD_MODE);	/* used to understand operational mode */

	LOG_INF(TAG "setup complete");
	return (osdp_t *) ctx;

 malloc_err:
	osdp_pd_teardown((osdp_t *) ctx);
	return NULL;
}

void osdp_pd_teardown(osdp_t *ctx)
{
	if (ctx != NULL) {
		safe_free(to_pd(ctx, 0));
		safe_free(to_cp(ctx));
		safe_free(ctx);
	}
}

void osdp_pd_refresh(osdp_t *ctx)
{
	assert(ctx);
	struct osdp_pd *pd = to_current_pd(ctx);

	pd_phy_state_update(pd);
}

int osdp_pd_get_cmd(osdp_t *ctx, struct osdp_cmd *cmd)
{
	assert(ctx);
	struct osdp_cmd *f;
	struct osdp_pd *pd = to_current_pd(ctx);

	f = pd->queue.front;
	if (f == NULL)
		return -1;

	memcpy(cmd, f, sizeof(struct osdp_cmd));
	pd->queue.front = pd->queue.front->__next;
	osdp_slab_free(to_osdp(ctx)->cmd_slab, f);
	return 0;
}
