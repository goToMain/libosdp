/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_common.h"

#define call_fprt(fp, ...)          ({int r=-1; if(fp != NULL) r = fp(__VA_ARGS__); r;})
#define pd_set_output(p, c)         call_fprt((p)->cmd_cb.output, (c))
#define pd_set_led(p, c)            call_fprt((p)->cmd_cb.led,    (c))
#define pd_set_buzzer(p, c)         call_fprt((p)->cmd_cb.buzzer, (c))
#define pd_set_text(p, c)           call_fprt((p)->cmd_cb.text,   (c))
#define pd_set_comm_params(p, c)    call_fprt((p)->cmd_cb.comset, (c))

enum osdp_phy_state_e {
	PD_PHY_STATE_IDLE,
	PD_PHY_STATE_SEND_REPLY,
	PD_PHY_STATE_ERR,
};

/**
 * Returns:
 *  0: success
 * -1: error
 *  2: retry current command
 */
int pd_decode_command(struct osdp_pd *p, struct osdp_data *reply, uint8_t * buf, int len)
{
	int i, ret = -1, cmd_id, pos = 0;
	union cmd_all cmd;

	cmd_id = buf[pos++];
	len--;

	osdp_log(LOG_DEBUG, "PD: processing command: 0x%02x", cmd_id);

	switch (cmd_id) {
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
		cmd.output.output_no    = buf[pos++];
		cmd.output.control_code = buf[pos++];
		cmd.output.tmr_count    = buf[pos++];
		cmd.output.tmr_count   |= buf[pos++] << 8;
		if (pd_set_output(p, &cmd.output) != 0)
			break;
		reply->id = REPLY_OSTATR;
		ret = 0;
		break;
	case CMD_LED:
		if (len != 14)
			break;
		cmd.led.reader = buf[pos++];
		cmd.led.number = buf[pos++];

		cmd.led.temporary.control_code = buf[pos++];
		cmd.led.temporary.on_count     = buf[pos++];
		cmd.led.temporary.off_count    = buf[pos++];
		cmd.led.temporary.on_color     = buf[pos++];
		cmd.led.temporary.off_color    = buf[pos++];
		cmd.led.temporary.timer        = buf[pos++];
		cmd.led.temporary.timer       |= buf[pos++] << 8;

		cmd.led.permanent.control_code = buf[pos++];
		cmd.led.permanent.on_count     = buf[pos++];
		cmd.led.permanent.off_count    = buf[pos++];
		cmd.led.permanent.on_color     = buf[pos++];
		cmd.led.permanent.off_color    = buf[pos++];
		if (pd_set_led(p, &cmd.led) < 0)
			break;
		reply->id = REPLY_ACK;
		ret = 0;
	case CMD_BUZ:
		if (len != 5)
			break;
		cmd.buzzer.reader    = buf[pos++];
		cmd.buzzer.tone_code = buf[pos++];
		cmd.buzzer.on_count  = buf[pos++];
		cmd.buzzer.off_count = buf[pos++];
		cmd.buzzer.rep_count = buf[pos++];
		if (pd_set_buzzer(p, &cmd.buzzer) < 0)
			break;
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_TEXT:
		if (len < 7)
			break;
		cmd.text.reader     = buf[pos++];
		cmd.text.cmd        = buf[pos++];
		cmd.text.temp_time  = buf[pos++];
		cmd.text.offset_row = buf[pos++];
		cmd.text.offset_col = buf[pos++];
		cmd.text.offset_col = buf[pos++];
		cmd.text.length = buf[pos++];
		if (cmd.text.length > 32)
			break;
		for (i = 0; i < cmd.text.length; i++)
			cmd.text.data[i] = buf[pos++];
		if (pd_set_text(p, &cmd.text) < 0)
			break;
		reply->id = REPLY_ACK;
		ret = 0;
		break;
	case CMD_COMSET:
		if (len != 5)
			break;
		cmd.comset.addr  = buf[pos++];
		cmd.comset.baud  = buf[pos++];
		cmd.comset.baud |= buf[pos++] << 8;
		cmd.comset.baud |= buf[pos++] << 16;
		cmd.comset.baud |= buf[pos++] << 24;
		if (pd_set_comm_params(p, &cmd.comset) < 0)
			break;
		reply->id = REPLY_COM;
		ret = 0;
		break;
	case CMD_CHLNG:
		if (len != 8)
			break;
		osdp_sc_init(p);
		for (i=0; i<8; i++)
			p->sc.cp_random[i] = buf[pos++];
		reply->id = REPLY_CCRYPT;
		ret = 0;
		break;
	case CMD_SCRYPT:
		if (len != 16)
			break;
		for (i=0; i<16; i++)
			p->sc.cp_cryptogram[i] = buf[pos++];
		reply->id = REPLY_RMAC_I;
		ret = 0;
		break;
	default:
		break;
	}

	if (ret != 0) {
		reply->id = REPLY_NAK;
		reply->data[0] = OSDP_PD_NAK_RECORD;
		return -1;
	}

	return 0;
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int pd_build_reply(struct osdp_pd *p, struct osdp_data *reply, uint8_t * pkt)
{
	int i, len = 0;

	uint8_t *buf = phy_packet_get_data(p, pkt);
	uint8_t *smb = phy_packet_get_smb(p, pkt);

	osdp_log(LOG_DEBUG, "PD: build reply: 0x%02x", reply->id);

	switch (reply->id) {
	case REPLY_ACK:
		buf[len++] = reply->id;
		break;
	case REPLY_PDID:
		buf[len++] = reply->id;
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
		buf[len++] = reply->id;
		for (i = 0; i < CAP_SENTINEL; i++) {
			if (p->cap[i].function_code != i)
				continue;
			buf[len++] = i;
			buf[len++] = p->cap[i].compliance_level;
			buf[len++] = p->cap[i].num_items;
		}
		break;
	case REPLY_LSTATR:
		buf[len++] = reply->id;
		buf[len++] = isset_flag(p, PD_FLAG_TAMPER);
		buf[len++] = isset_flag(p, PD_FLAG_POWER);
		break;
	case REPLY_RSTATR:
		buf[len++] = reply->id;
		buf[len++] = isset_flag(p, PD_FLAG_R_TAMPER);
		break;
	case REPLY_COM:
		buf[len++] = reply->id;
		buf[len++] = byte_0(p->baud_rate);
		buf[len++] = byte_1(p->baud_rate);
		buf[len++] = byte_2(p->baud_rate);
		buf[len++] = byte_3(p->baud_rate);
		break;
	case REPLY_NAK:
		buf[len++] = reply->id;
		buf[len++] = (reply->len > sizeof(struct osdp_data)) ?
					reply->data[0] : OSDP_PD_NAK_RECORD;
	case REPLY_CCRYPT:
		if (smb == NULL)
			break;
		osdp_fill_random(p->sc.pd_random, 8);
		osdp_compute_session_keys(to_ctx(p));
		osdp_compute_pd_cryptogram(p);
		buf[len++] = REPLY_CCRYPT;
		for (i = 0; i < 8; i++)
			buf[len++] = p->sc.pd_client_uid[i];
		for (i = 0; i < 8; i++)
			buf[len++] = p->sc.pd_random[i];
		for (i = 0; i < 16; i++)
			buf[len++] = p->sc.pd_cryptogram[i];
		smb[1] = SCS_12;
		smb[2] = isset_flag(p, PD_FLAG_SC_USE_SCBKD) ? 1 : 0;
		break;
	case REPLY_RMAC_I:
		if (smb == NULL)
			break;
		osdp_compute_rmac_i(p);
		buf[len++] = REPLY_RMAC_I;
		for (i = 0; i < 16; i++)
			buf[len++] = p->sc.r_mac[i];
		smb[1] = SCS_14;
		if (osdp_verify_cp_cryptogram(p) == 0)
			smb[2] = 0x01;
		else
			smb[2] = 0x00;
		set_flag(p, PD_FLAG_SC_ACTIVE);
		clear_flag(p, PD_FLAG_SC_USE_SCBKD);
		clear_flag(p, PD_FLAG_INSTALL_MODE);
		break;
	}

	if (smb && (smb[1] > SCS_14) && isset_flag(p, PD_FLAG_SC_ACTIVE))
		smb[1] = (len > 1) ? SCS_17 : SCS_15;

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
int pd_send_reply(struct osdp_pd *p, struct osdp_data *reply)
{
	int ret, len;
	uint8_t buf[OSDP_PACKET_BUF_SIZE];

	/* init packet buf with header */
	len = phy_build_packet_head(p, reply->id, buf, OSDP_PACKET_BUF_SIZE);
	if (len < 0) {
		osdp_log(LOG_ERR, "PD: failed at phy_build_packet_head");
		return -1;
	}

	/* fill reply data */
	ret = pd_build_reply(p, reply, buf);
	if (ret <= 0) {
		osdp_log(LOG_ERR, "PD: failed at pd_build_reply %d", reply->id);
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = phy_build_packet_tail(p, buf, len, OSDP_PACKET_BUF_SIZE);
	if (len < 0) {
		osdp_log(LOG_ERR, "PD: failed to build reply %d", reply->id);
		return -1;
	}

	ret = p->send_func(buf, len);

	return (ret == len) ? 0 : -1;
}

/**
 * pd_process_command - received buffer from serial stream handling partials
 * Returns:
 *  0: success
 * -1: error
 *  1: no data yet
 *  2: re-issue command
 */
int pd_process_command(struct osdp_pd *p, struct osdp_data *reply)
{
	int ret;

	ret = p->recv_func(p->phy_rx_buf + p->phy_rx_buf_len,
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

	ret = phy_decode_packet(p, p->phy_rx_buf, p->phy_rx_buf_len);
	if (ret < 0) {
		osdp_log(LOG_ERR, "PD: failed to decode response");
		return -1;
	}

	ret = pd_decode_command(p, reply, p->phy_rx_buf, ret);
	p->phy_rx_buf_len = 0;

	return ret;
}

/**
 * Returns:
 *  -1: phy is in error state. Main state machine must reset it.
 *   0: Success
 */
int pd_phy_state_update(struct osdp_pd *pd)
{
	int ret = 0;
	uint8_t phy_reply[OSDP_DATA_BUF_SIZE];
	struct osdp_data *reply = (struct osdp_data *)phy_reply;

	switch (pd->phy_state) {
	case PD_PHY_STATE_IDLE:
		ret = pd_process_command(pd, reply);
		if (ret == 1)	/* no data; wait */
			break;
		if (ret < 0) {	/* error */
			pd->phy_state = PD_PHY_STATE_ERR;
			break;
		}
		ret = 1;
		pd->phy_state = PD_PHY_STATE_SEND_REPLY;
		/* no break */
	case PD_PHY_STATE_SEND_REPLY:
		if ((ret = pd_send_reply(pd, reply)) == 0) {
			pd->phy_state = PD_PHY_STATE_IDLE;
			break;
		}
		if (ret == -1) {	/* send failed! */
			pd->phy_state = PD_PHY_STATE_ERR;
			break;
		}
		pd->phy_state = PD_PHY_STATE_IDLE;
		break;
	case PD_PHY_STATE_ERR:
		ret = -1;
		break;
	}

	return ret;
}

osdp_pd_t *osdp_pd_setup(int num_pd, osdp_pd_info_t * p)
{
	int fc;
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;
	struct pd_cap *cap;

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		osdp_log(LOG_ERR, "PD: Failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		osdp_log(LOG_ERR, "PD: Failed to alloc struct osdp_cp");
		goto malloc_err;
	}
	cp = to_cp(ctx);
	node_set_parent(cp, ctx);
	cp->num_pd = 1;

	ctx->pd = calloc(1, sizeof(struct osdp_pd));
	if (ctx->pd == NULL) {
		osdp_log(LOG_ERR, "PD: Failed to alloc struct osdp_pd");
		goto malloc_err;
	}
	set_current_pd(ctx, 0);
	pd = to_pd(ctx, 0);

	node_set_parent(pd, ctx);
	pd->baud_rate = p->baud_rate;
	pd->address = p->address;
	pd->flags = p->init_flags;
	pd->seq_number = -1;
	pd->send_func = p->send_func;
	pd->recv_func = p->recv_func;
	memcpy(&pd->id, &p->id, sizeof(struct pd_id));
	cap = p->cap;
	while (cap && ((fc = cap->function_code) > 0)) {
		if (fc >= CAP_SENTINEL)
			break;
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}

	set_flag(pd, PD_FLAG_PD_MODE);	/* used to understand operational mode */

	osdp_set_log_level(LOG_WARNING);
	osdp_log(LOG_INFO, "pd setup complete");
	return (osdp_pd_t *) ctx;

 malloc_err:
	osdp_pd_teardown((osdp_pd_t *) ctx);
	return NULL;
}

void osdp_pd_teardown(osdp_pd_t *ctx)
{
	struct osdp_cp *cp;
	struct osdp_pd *pd;

	if (ctx == NULL)
		return;

	/* teardown PD */
	pd = to_pd(ctx, 0);
	if (pd != NULL) {
		free(pd);
	}

	/* teardown CP */
	cp = to_cp(ctx);
	if (cp != NULL) {
		free(cp);
	}

	free(ctx);
}

void osdp_pd_refresh(osdp_pd_t *ctx)
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd_phy_state_update(pd);
}

void osdp_pd_set_callback_cmd_led(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_led *p))
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd->cmd_cb.led = cb;
}

void osdp_pd_set_callback_cmd_buzzer(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_buzzer *p))
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd->cmd_cb.buzzer = cb;
}

void osdp_pd_set_callback_cmd_text(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_text *p))
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd->cmd_cb.text = cb;
}

void osdp_pd_set_callback_cmd_output(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_output *p))
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd->cmd_cb.output = cb;
}

void osdp_pd_set_callback_cmd_comset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_comset *p))
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd->cmd_cb.comset = cb;
}