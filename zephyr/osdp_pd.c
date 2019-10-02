/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>
#include <init.h>

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

static struct osdp g_osdp_context;

/**
 * Returns:
 *  0: success
 * -1: error
 *  2: retry current command
 */
int pd_decode_command(struct osdp_pd *p, struct osdp_data *reply, u8_t * buf, int len)
{
	int i, ret = -1, cmd_id, pos = 0;
	union cmd_all cmd;

	cmd_id = buf[pos++];
	len--;

	printk("Proc cmd: 0x%02x\n", cmd_id);

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
		ret = 0;
		reply->id = REPLY_PDID;
		break;
	case CMD_CAP:
		pos++;		// Skip reply type info.
		ret = 0;
		reply->id = REPLY_PDCAP;
		break;
	case CMD_OUT:
		if (len != 4)
			break;
		cmd.output.output_no = buf[pos++];
		cmd.output.control_code = buf[pos++];
		cmd.output.tmr_count = buf[pos++];
		cmd.output.tmr_count |= buf[pos++] << 8;
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
		cmd.led.temporary.on_count = buf[pos++];
		cmd.led.temporary.off_count = buf[pos++];
		cmd.led.temporary.on_color = buf[pos++];
		cmd.led.temporary.off_color = buf[pos++];
		cmd.led.temporary.timer = buf[pos++];
		cmd.led.temporary.timer |= buf[pos++] << 8;

		cmd.led.permanent.control_code = buf[pos++];
		cmd.led.permanent.on_count = buf[pos++];
		cmd.led.permanent.off_count = buf[pos++];
		cmd.led.permanent.on_color = buf[pos++];
		cmd.led.permanent.off_color = buf[pos++];
		if (pd_set_led(p, &cmd.led) < 0)
			break;
		reply->id = REPLY_ACK;
		ret = 0;
	case CMD_BUZ:
		if (len != 5)
			break;
		cmd.buzzer.reader = buf[pos++];
		cmd.buzzer.tone_code = buf[pos++];
		cmd.buzzer.on_count = buf[pos++];
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
		cmd.text.reader = buf[pos++];
		cmd.text.cmd = buf[pos++];
		cmd.text.temp_time = buf[pos++];
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
		cmd.comset.addr = buf[pos++];
		cmd.comset.baud = buf[pos++];
		cmd.comset.baud |= buf[pos++] << 8;
		cmd.comset.baud |= buf[pos++] << 16;
		cmd.comset.baud |= buf[pos++] << 24;
		if (pd_set_comm_params(p, &cmd.comset) < 0)
			break;
		reply->id = REPLY_COM;
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
int pd_build_reply(struct osdp_pd *p, struct osdp_data *reply, u8_t * buf, int maxlen)
{
	int i, len = 0;

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
	default:
		buf[len++] = REPLY_NAK;
		buf[len++] = OSDP_PD_NAK_SC_UNSUP;
		break;
	}
	return len;
}

int osdp_uart_send(struct osdp_pd *p, u8_t *buf, int len)
{
	int sent = 0;

	while (sent < len) {
		uart_poll_out(p->uart_dev, buf[sent]);
		sent++;
	}

	return sent;
}

int pd_send_reply(struct osdp_pd *p, struct osdp_data *reply)
{
	int ret, len;
	u8_t buf[512];

	if ((len = phy_build_packet_head(p, buf, 512)) < 0) {
		printk("failed to phy_build_packet_head\n");
		return -1;
	}

	if ((ret = pd_build_reply(p, reply, buf + len, 512 - len)) < 0) {
		printk("failed to build command %d\n", reply->id);
		return -1;
	}
	len += ret;

	if ((len = phy_build_packet_tail(p, buf, len, 512)) < 0) {
		printk("failed to build command %d\n", reply->id);
		return -1;
	}

	ret = osdp_uart_send(p, buf, len);

	return (ret == len) ? 0 : -1;
}

/**
 * Returns:
 *  0: success
 * -1: error
 *  1: no data yet
 *  2: re-issue command
 */
int pd_process_command(struct osdp_pd *p, struct osdp_data *reply)
{
	int ret, len;
	k_spinlock_key_t key;
	u8_t cmd_buf[OSDP_PD_RX_BUF_LENGTH];

	ret = phy_check_packet(p->rx_data, p->rx_len);

	/* rx_data had invalid MARK or SOM. Reset the receive buf */
	if (ret < 0) {
		key = k_spin_lock(&p->rx_lock);
		p->rx_len = 0;
		k_spin_unlock(&p->rx_lock, key);
		return 1;
	}

	/* rx_len != pkt->len; wait for more data */
	if (ret > 0)
		return 1;

	/* copy and empty rx buffer */
	key = k_spin_lock(&p->rx_lock);
	len = p->rx_len;
	memcpy(cmd_buf, p->rx_data, len);
	p->rx_len = 0;
	k_spin_unlock(&p->rx_lock, key);

	if ((len = phy_decode_packet(p, cmd_buf, len)) < 0) {
		printk("failed decode response\n");
	}

	return pd_decode_command(p, reply, cmd_buf, len);
}

/**
 * Returns:
 *  -1: phy is in error state. Main state machine must reset it.
 *   0: Success
 */
int pd_phy_state_update(struct osdp_pd *pd)
{
	int ret = 0;
	u8_t reply_buf[128];
	struct osdp_data *reply;

	reply = (struct osdp_data *)reply_buf;

	switch (pd->phy_state) {
	case PD_PHY_STATE_IDLE:
		ret = pd_process_command(pd, reply);
		if (ret == 1)
			break;
		if (ret < 0) {
			printk("command dequeue error\n");
			pd->phy_state = PD_PHY_STATE_ERR;
			break;
		}
		ret = 1;
		pd->phy_state = PD_PHY_STATE_SEND_REPLY;
		break;
	case PD_PHY_STATE_SEND_REPLY:
		if ((ret = pd_send_reply(pd, reply)) == 0) {
			pd->phy_state = PD_PHY_STATE_IDLE;
			break;
		}
		if (ret == -1) {
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

void osdp_uart_isr(struct device *dev)
{
	int rx;
	struct osdp_pd *pd = g_osdp_context.pd;

	k_spinlock_key_t key = k_spin_lock(&pd->rx_lock);
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (!uart_irq_rx_ready(dev))
			continue;

		/* Character(s) have been received */

		if (pd->rx_len > OSDP_PD_RX_BUF_LENGTH)
			return;

		rx = uart_fifo_read(dev, pd->rx_data + pd->rx_len,
				    OSDP_PD_RX_BUF_LENGTH - pd->rx_len);
		if (rx  <  0)
			continue;

		pd->rx_len += rx;
	}
	k_spin_unlock(&pd->rx_lock, key);
}

int osdp_pd_setup(struct osdp_pd_info * p)
{
	struct osdp *ctx = &g_osdp_context;
	struct osdp_cp *cp = to_cp(ctx);
	struct osdp_pd *pd = to_pd(ctx, 0);

	set_current_pd(ctx, 0);
	child_set_parent(cp, ctx);
	child_set_parent(pd, ctx);

	pd->baud_rate = p->baud_rate;
	pd->address = p->address;
	pd->flags = p->init_flags;
	pd->seq_number = -1;
	pd->phy_state = PD_PHY_STATE_IDLE;
	memcpy(&pd->id, &p->id, sizeof(struct pd_id));

	struct pd_cap *cap = pd->cap;
	while (cap && cap->function_code > 0) {
		int fc = pd->cap->function_code;
		if (fc >= CAP_SENTINEL)
			break;
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}

	set_flag(pd, PD_FLAG_PD_MODE);

	/* OSDP UART init */
	u8_t c;
	pd->uart_dev =  device_get_binding(CONFIG_OSDP_UART_DEV_NAME);
	uart_irq_rx_disable(pd->uart_dev);
	uart_irq_tx_disable(pd->uart_dev);
	uart_irq_callback_set(pd->uart_dev, osdp_uart_isr);
	/* Drain the fifo */
	while (uart_irq_rx_ready(pd->uart_dev)) {
		uart_fifo_read(pd->uart_dev, &c, 1);
	}
	uart_irq_rx_enable(pd->uart_dev);

	return 0;
}

void osdp_pd_set_callback_cmd_led(int (*cb) (struct osdp_cmd_led *p))
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd->cmd_cb.led = cb;
}

void osdp_pd_set_callback_cmd_buzzer(int (*cb) (struct osdp_cmd_buzzer *p))
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd->cmd_cb.buzzer = cb;
}

void osdp_pd_set_callback_cmd_text(int (*cb) (struct osdp_cmd_text *p))
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd->cmd_cb.text = cb;
}

void osdp_pd_set_callback_cmd_output(int (*cb) (struct osdp_cmd_output *p))
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd->cmd_cb.output = cb;
}

void osdp_pd_set_callback_cmd_comset(int (*cb) (struct osdp_cmd_comset *p))
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd->cmd_cb.comset = cb;
}

void osdp_pd_refresh()
{
	struct osdp_pd *pd = g_osdp_context.pd;

	pd_phy_state_update(pd);
}

