/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <utils/utils.h>

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
#define CMD_MFG_LEN                    4   /* variable length command */
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
#define REPLY_MFGREP_LEN               4   /* variable length command */
#define REPLY_CCRYPT_DATA_LEN          32
#define REPLY_RMAC_I_DATA_LEN          16
#define REPLY_KEYPPAD_DATA_LEN         2   /* variable length command */
#define REPLY_RAW_DATA_LEN             4   /* variable length command */
#define REPLY_FMT_DATA_LEN             3   /* variable length command */
#define REPLY_BUSY_DATA_LEN            0

#define OSDP_CP_ERR_GENERIC           -1
#define OSDP_CP_ERR_NO_DATA            1
#define OSDP_CP_ERR_RETRY_CMD          2
#define OSDP_CP_ERR_CAN_YIELD          3
#define OSDP_CP_ERR_INPROG             4

struct cp_cmd_node {
	queue_node_t node;
	struct osdp_cmd object;
};

static int cp_cmd_queue_init(struct osdp_pd *pd)
{
	if (slab_init(&pd->cmd.slab, sizeof(struct cp_cmd_node),
		      OSDP_CP_CMD_POOL_SIZE)) {
		LOG_ERR("Failed to initialize command slab");
		return -1;
	}
	queue_init(&pd->cmd.queue);
	return 0;
}

static void cp_cmd_queue_del(struct osdp_pd *pd)
{
	slab_del(&pd->cmd.slab);
}

static struct osdp_cmd *cp_cmd_alloc(struct osdp_pd *pd)
{
	struct cp_cmd_node *cmd = NULL;

	if (slab_alloc(&pd->cmd.slab, (void **)&cmd)) {
		LOG_ERR("Memory allocation failed");
		return NULL;
	}
	return &cmd->object;
}

static void cp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct cp_cmd_node *n;

	n = CONTAINER_OF(cmd, struct cp_cmd_node, object);
	slab_free(&pd->cmd.slab, n);
}

static void cp_cmd_enqueue(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct cp_cmd_node *n;

	n = CONTAINER_OF(cmd, struct cp_cmd_node, object);
	queue_enqueue(&pd->cmd.queue, &n->node);
}

static int cp_cmd_dequeue(struct osdp_pd *pd, struct osdp_cmd **cmd)
{
	struct cp_cmd_node *n;
	queue_node_t *node;

	if (queue_dequeue(&pd->cmd.queue, &node)) {
		return -1;
	}
	n = CONTAINER_OF(node, struct cp_cmd_node, node);
	*cmd = &n->object;
	return 0;
}

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
		return OSDP_CP_ERR_GENERIC;
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
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.timer_count);
		buf[len++] = BYTE_1(cmd->output.timer_count);
		ret = 0;
		break;
	case CMD_LED:
		if (max_len < CMD_LED_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->led.reader;
		buf[len++] = cmd->led.led_number;

		buf[len++] = cmd->led.temporary.control_code;
		buf[len++] = cmd->led.temporary.on_count;
		buf[len++] = cmd->led.temporary.off_count;
		buf[len++] = cmd->led.temporary.on_color;
		buf[len++] = cmd->led.temporary.off_color;
		buf[len++] = BYTE_0(cmd->led.temporary.timer_count);
		buf[len++] = BYTE_1(cmd->led.temporary.timer_count);

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
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.control_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		ret = 0;
		break;
	case CMD_TEXT:
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		if (max_len < (CMD_TEXT_LEN + cmd->text.length)) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.control_code;
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
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->comset.address;
		buf[len++] = BYTE_0(cmd->comset.baud_rate);
		buf[len++] = BYTE_1(cmd->comset.baud_rate);
		buf[len++] = BYTE_2(cmd->comset.baud_rate);
		buf[len++] = BYTE_3(cmd->comset.baud_rate);
		ret = 0;
		break;
	case CMD_MFG:
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		if (max_len < (CMD_MFG_LEN + cmd->mfg.length)) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(cmd->mfg.vendor_code);
		buf[len++] = BYTE_1(cmd->mfg.vendor_code);
		buf[len++] = BYTE_2(cmd->mfg.vendor_code);
		buf[len++] = cmd->mfg.command;
		for (i = 0; i < cmd->mfg.length; i++) {
			buf[len++] = cmd->mfg.data[i];
		}
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
		osdp_get_rand(pd->sc.cp_random, 8);
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
		LOG_ERR(TAG "Unknown/Unsupported command %02x", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
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
		LOG_ERR(TAG "Unable to build command %02x", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

	return len;
}

static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp_cp *cp = TO_CTX(pd)->cp;
	int i, ret = OSDP_CP_ERR_GENERIC, pos = 0, t1, t2;
	struct osdp_event event;

	if (len < 1) {
		LOG_ERR("response must have at least one byte");
		return OSDP_CP_ERR_GENERIC;
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
			t1 = buf[pos++]; /* func_code */
			if (t1 > OSDP_PD_CAP_SENTINEL) {
				break;
			}
			pd->cap[t1].function_code    = t1;
			pd->cap[t1].compliance_level = buf[pos++];
			pd->cap[t1].num_items        = buf[pos++];
		}
		/* post-capabilities hooks */
		t2 = OSDP_PD_CAP_COMMUNICATION_SECURITY;
		if (pd->cap[t2].compliance_level & 0x01)
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
		} else {
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
		LOG_WRN(TAG "COMSET responded with ID:%d baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		ret = 0;
		break;
	case REPLY_KEYPPAD:
		if (len < REPLY_KEYPPAD_DATA_LEN || !cp->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_KEYPRESS;
		event.keypress.reader_no = buf[pos++];
		event.keypress.length    = buf[pos++]; /* key length */
		if ((len - REPLY_KEYPPAD_DATA_LEN) != event.keypress.length) {
			break;
		}
		for (i = 0; i < event.keypress.length; i++) {
			event.keypress.data[i] = buf[pos + i];
		}
		cp->event_callback(cp->event_callback_arg, pd->offset, &event);
		ret = 0;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN || !cp->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.format    = buf[pos++];
		event.cardread.length    = buf[pos++];       /* bits LSB */
		event.cardread.length   |= buf[pos++] << 8;  /* bits MSB */
		event.cardread.direction = 0;                /* un-specified */
		t1 = (event.cardread.length + 7) / 8;        /* len: bytes */
		if (t1 != (len - REPLY_RAW_DATA_LEN)) {
			break;
		}
		for (i = 0; i < t1; i++) {
			event.cardread.data[i] = buf[pos + i];
		}
		cp->event_callback(cp->event_callback_arg, pd->offset, &event);
		ret = 0;
		break;
	case REPLY_FMT:
		if (len < REPLY_FMT_DATA_LEN || !cp->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.direction = buf[pos++];
		event.cardread.length    = buf[pos++];
		event.cardread.format    = OSDP_CARD_FMT_ASCII;
		if (event.cardread.length != (len - REPLY_FMT_DATA_LEN) ||
		    event.cardread.length > OSDP_EVENT_MAX_DATALEN) {
			break;
		}
		for (i = 0; i < event.cardread.length; i++) {
			event.cardread.data[i] = buf[pos + i];
		}
		cp->event_callback(cp->event_callback_arg, pd->offset, &event);
		ret = 0;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		if (len != REPLY_BUSY_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_RETRY_CMD;
		break;
	case REPLY_MFGREP:
		if (len < REPLY_MFGREP_LEN || !cp->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_MFGREP;
		event.mfgrep.vendor_code  = buf[pos++];
		event.mfgrep.vendor_code |= buf[pos++] << 8;
		event.mfgrep.vendor_code |= buf[pos++] << 16;
		event.mfgrep.command      = buf[pos++];
		event.mfgrep.length       = len - REPLY_MFGREP_LEN;
		if (event.mfgrep.length > OSDP_EVENT_MAX_DATALEN) {
			break;
		}
		for (i = 0; i < event.mfgrep.length; i++) {
			event.mfgrep.data[i] = buf[pos + i];
		}
		cp->event_callback(cp->event_callback_arg, pd->offset, &event);
		ret = 0;
		break;
	case REPLY_CCRYPT:
		if (len != REPLY_CCRYPT_DATA_LEN) {
			break;
		}
		for (i = 0; i < 8; i++) {
			pd->sc.pd_client_uid[i] = buf[pos++];
		}
		for (i = 0; i < 8; i++) {
			pd->sc.pd_random[i] = buf[pos++];
		}
		for (i = 0; i < 16; i++) {
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
	default:
		LOG_DBG(TAG "unexpected reply: 0x%02x", pd->reply_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (ret == OSDP_CP_ERR_GENERIC) {
		LOG_ERR(TAG "REPLY %02x for CMD %02x format error!",
			pd->cmd_id, pd->reply_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "CMD: %02x REPLY: %02x", pd->cmd_id, pd->reply_id);
	}

	return ret;
}

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

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG(TAG "bytes sent");
			osdp_dump(NULL, pd->rx_buf, len);
		}
	}

	return (ret == len) ? 0 : -1;
}

static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int rec_bytes, ret, max_len;

	buf = pd->rx_buf + pd->rx_buf_len;
	max_len = sizeof(pd->rx_buf) - pd->rx_buf_len;

	rec_bytes = pd->channel.recv(pd->channel.data, buf, max_len);
	if (rec_bytes <= 0) {	/* No data received */
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len += rec_bytes;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG(TAG "bytes received");
			osdp_dump(NULL, pd->rx_buf, pd->rx_buf_len);
		}
	}

	/* Valid OSDP packet in buffer */
	ret = osdp_phy_decode_packet(pd, pd->rx_buf, pd->rx_buf_len);
	if (ret == OSDP_ERR_PKT_FMT) {
		return -1; /* fatal errors */
	} else if (ret == OSDP_ERR_PKT_WAIT) {
		/* rx_buf_len != pkt->len; wait for more data */
		return OSDP_CP_ERR_NO_DATA;
	} else if (ret == OSDP_ERR_PKT_SKIP) {
		/* soft fail - discard this message */
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len = ret;

	return cp_decode_response(pd, pd->rx_buf, pd->rx_buf_len);
}

static void cp_flush_command_queue(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	while (cp_cmd_dequeue(pd, &cmd) == 0) {
		cp_cmd_free(pd, cmd);
	}
}

static inline void cp_set_offline(struct osdp_pd *pd)
{
	pd->state = OSDP_CP_STATE_OFFLINE;
	pd->tstamp = osdp_millis_now();
	CLEAR_FLAG(pd, PD_FLAG_SC_ACTIVE);
}

static inline void cp_set_state(struct osdp_pd *pd, enum osdp_state_e state)
{
	pd->state = state;
	CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
}

/**
 * Note: This method must not dequeue cmd unless it reaches an invalid state.
 */
static int cp_phy_state_update(struct osdp_pd *pd)
{
	int ret = OSDP_CP_ERR_INPROG, tmp;
	struct osdp_cmd *cmd = NULL;

	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_ERR_WAIT:
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_IDLE:
		if (cp_cmd_dequeue(pd, &cmd)) {
			ret = 0;
			break;
		}
		pd->cmd_id = cmd->id;
		memcpy(pd->ephemeral_data, cmd, sizeof(struct osdp_cmd));
		cp_cmd_free(pd, cmd);
		/* fall-thru */
	case OSDP_CP_PHY_STATE_SEND_CMD:
		if ((cp_send_command(pd)) < 0) {
			LOG_ERR(TAG "send command error");
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		tmp = cp_process_reply(pd);
		if (tmp == 0) { /* success */
			pd->phy_state = OSDP_CP_PHY_STATE_CLEANUP;
			break;
		}
		if (tmp == OSDP_CP_ERR_RETRY_CMD) {
			LOG_INF(TAG "PD busy; retry last command");
			pd->phy_tstamp = osdp_millis_now();
			pd->phy_state = OSDP_CP_PHY_STATE_WAIT;
			ret = 2;
			break;
		}
		if (tmp == OSDP_CP_ERR_GENERIC) {
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			break;
		}
		if (osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			LOG_ERR(TAG "CMD: %02x - response timeout", pd->cmd_id);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
		}
		break;
	case OSDP_CP_PHY_STATE_WAIT:
		if (osdp_millis_since(pd->phy_tstamp) < OSDP_CMD_RETRY_WAIT_MS) {
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		cp_flush_command_queue(pd);
		pd->phy_state = OSDP_CP_PHY_STATE_ERR_WAIT;
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_CLEANUP:
		pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
		ret = OSDP_CP_ERR_CAN_YIELD; /* in between commands */
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

	c = cp_cmd_alloc(pd);
	if (c == NULL) {
		return OSDP_CP_ERR_GENERIC;
	}

	c->id = cmd;
	cp_cmd_enqueue(pd, c);
	SET_FLAG(pd, PD_FLAG_AWAIT_RESP);
	return OSDP_CP_ERR_INPROG;
}

static int state_update(struct osdp_pd *pd)
{
	int phy_state, soft_fail;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == OSDP_CP_ERR_INPROG ||
	    phy_state == OSDP_CP_ERR_CAN_YIELD) {
		return OSDP_CP_ERR_GENERIC;
	}

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->state == OSDP_CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->state != OSDP_CP_STATE_OFFLINE &&
	    phy_state == OSDP_CP_ERR_GENERIC && soft_fail == 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successful */

	switch (pd->state) {
	case OSDP_CP_STATE_ONLINE:
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)  == false &&
		    ISSET_FLAG(pd, PD_FLAG_SC_CAPABLE) == true  &&
		    osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_MS) {
			LOG_INF(TAG "Retry SC after retry timeout");
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
		if (osdp_millis_since(pd->tstamp) > OSDP_CMD_RETRY_WAIT_MS) {
			cp_set_state(pd, OSDP_CP_STATE_INIT);
			osdp_phy_state_reset(pd);
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
			CLEAR_FLAG(pd, PD_FLAG_SC_SCBKD_DONE);
			CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
		if (ISSET_FLAG(pd, OSDP_FLAG_ENFORCE_SECURE)) {
			LOG_INF(TAG "PD is not SC capable. Set PD offline due "
				    "to ENFORCE_SECURE");
			cp_set_offline(pd);
		} else {
			cp_set_state(pd, OSDP_CP_STATE_ONLINE);
		}
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
			if (ISSET_FLAG(pd, OSDP_FLAG_ENFORCE_SECURE)) {
				LOG_INF(TAG "SC Failed. Set PD offline due to "
				        "ENFORCE_SECURE");
				cp_set_offline(pd);
				break;
			}
			if (ISSET_FLAG(pd, PD_FLAG_SC_SCBKD_DONE)) {
				LOG_INF(TAG "SC Failed. Online without SC");
				pd->sc_tstamp = osdp_millis_now();
				cp_set_state(pd, OSDP_CP_STATE_ONLINE);
				break;
			}
			SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			SET_FLAG(pd, PD_FLAG_SC_SCBKD_DONE);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			pd->phy_state = 0; /* soft reset phy state */
			LOG_WRN(TAG "SC Failed. Retry with SCBK-D");
			break;
		}
		if (pd->reply_id != REPLY_CCRYPT) {
			if (ISSET_FLAG(pd, OSDP_FLAG_ENFORCE_SECURE)) {
				LOG_ERR(TAG "CHLNG failed. Set PD offline due to "
				        "ENFORCE_SECURE");
				cp_set_offline(pd);
				break;
			} else {
				LOG_ERR(TAG "CHLNG failed. Online without SC");
				pd->sc_tstamp = osdp_millis_now();
				cp_set_state(pd, OSDP_CP_STATE_ONLINE);
			}
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
			LOG_WRN(TAG "SC ACtive with SCBK-D. Set SCBK");
			cp_set_state(pd, OSDP_CP_STATE_SET_SCBK);
			break;
		}
		LOG_INF(TAG "SC Active");
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

static int osdp_cp_send_command_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p)
{
	int i;
	struct osdp_cmd *cmd;
	struct osdp_pd *pd;

	if (osdp_get_sc_status_mask(ctx) != PD_MASK(ctx)) {
		LOG_WRN(TAG "CMD_KEYSET can be sent only when all PDs are "
			"ONLINE and SC_ACTIVE.");
		return 1;
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = TO_PD(ctx, i);
		cmd = cp_cmd_alloc(pd);
		if (cmd == NULL) {
			return -1;
		}
		cmd->id = CMD_KEYSET;
		memcpy(&cmd->keyset, p, sizeof(struct osdp_cmd_keyset));
		cp_cmd_enqueue(pd, cmd);
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
		return NULL;
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

	ctx->pd = calloc(1, sizeof(struct osdp_pd) * num_pd);
	if (ctx->pd == NULL) {
		LOG_ERR(TAG "failed to alloc struct osdp_pd[]");
		goto error;
	}
	cp->num_pd = num_pd;

	for (i = 0; i < num_pd; i++) {
		osdp_pd_info_t *p = info + i;
		pd = TO_PD(ctx, i);
		pd->offset = i;
		pd->__parent = ctx;
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->flags;
		pd->seq_number = -1;
		if (cp_cmd_queue_init(pd)) {
			goto error;
		}
		memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));
	}
	SET_CURRENT_PD(ctx, 0);
	LOG_INF(TAG "setup complete");
	return (osdp_t *) ctx;

error:
	osdp_cp_teardown((osdp_t *)ctx);
	return NULL;
}

OSDP_EXPORT
void osdp_cp_teardown(osdp_t *ctx)
{
	int i;

	if (ctx == NULL || TO_CP(ctx) == NULL) {
		return;
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		cp_cmd_queue_del(TO_PD(ctx, i));
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
		state_update(GET_CURRENT_PD(ctx));
	}
}

OSDP_EXPORT void
osdp_cp_set_event_callback(osdp_t *ctx, cp_event_callback_t cb, void *arg)
{
	assert(ctx);

	TO_CP(ctx)->event_callback = cb;
	TO_CP(ctx)->event_callback_arg = arg;
}

OSDP_EXPORT
int osdp_cp_send_command(osdp_t *ctx, int pd, struct osdp_cmd *p)
{
	assert(ctx);
	struct osdp_cmd *cmd;
	int cmd_id;

	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}
	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}

	switch (p->id) {
	case OSDP_CMD_OUTPUT:
		cmd_id = CMD_OUT;
		break;
	case OSDP_CMD_LED:
		cmd_id = CMD_LED;
		break;
	case OSDP_CMD_BUZZER:
		cmd_id = CMD_BUZ;
		break;
	case OSDP_CMD_TEXT:
		cmd_id = CMD_TEXT;
		break;
	case OSDP_CMD_COMSET:
		cmd_id = CMD_COMSET;
		break;
	case OSDP_CMD_MFG:
		cmd_id = CMD_MFG;
		break;
	case OSDP_CMD_KEYSET:
		return osdp_cp_send_command_keyset(ctx, &p->keyset);
	default:
		LOG_ERR(TAG "Invalid command ID");
		return -1;
	}

	cmd = cp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	memcpy(cmd, p, sizeof(struct osdp_cmd));
	cmd->id = cmd_id; /* translate to internal */
	cp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}

#ifdef UNIT_TESTING

/**
 * Force export some private methods for testing.
 */
void (*test_cp_cmd_enqueue)(struct osdp_pd *, struct osdp_cmd *) = cp_cmd_enqueue;
struct osdp_cmd * (*test_cp_cmd_alloc)(struct osdp_pd *) = cp_cmd_alloc;
int (*test_cp_phy_state_update)(struct osdp_pd *) = cp_phy_state_update;
int (*test_state_update)(struct osdp_pd *) = state_update;

#endif /* UNIT_TESTING */
