/*
 * Copyright (c) 2019-2025 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <utils/disjoint_set.h>

#include "osdp_common.h"
#include "osdp_file.h"
#include "osdp_diag.h"

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
#define CMD_MFG_LEN                    4   /* variable length command */

#define REPLY_ACK_DATA_LEN             0
#define REPLY_PDID_DATA_LEN            12
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_DATA_LEN          2
#define REPLY_RSTATR_DATA_LEN          1
#define REPLY_COM_DATA_LEN             5
#define REPLY_NAK_DATA_LEN             1
#define REPLY_CCRYPT_DATA_LEN          32
#define REPLY_RMAC_I_DATA_LEN          16
#define REPLY_KEYPAD_DATA_LEN          2   /* variable length command */
#define REPLY_RAW_DATA_LEN             4   /* variable length command */
#define REPLY_BUSY_DATA_LEN            0
#define REPLY_MFGREP_LEN               3   /* variable length command */

enum osdp_cp_error_e {
	OSDP_CP_ERR_NONE = 0,
	OSDP_CP_ERR_GENERIC = -1,
	OSDP_CP_ERR_NO_DATA = -2,
	OSDP_CP_ERR_RETRY_CMD = -3,
	OSDP_CP_ERR_CAN_YIELD = -4,
	OSDP_CP_ERR_INPROG = -5,
	OSDP_CP_ERR_UNKNOWN = -6,
	OSDP_CP_ERR_SEQ_NUM = -7,
	OSDP_CP_ERR_APP = -8, /* Application layer error */
};

struct cp_cmd_node {
	queue_node_t node;
	struct osdp_cmd object;
};

static int cp_cmd_queue_init(struct osdp_pd *pd)
{
	if (slab_init(&pd->app_data.slab,
		      sizeof(struct cp_cmd_node),
		      pd->app_data.slab_blob,
		      sizeof(pd->app_data.slab_blob)) < 0) {
		LOG_ERR("Failed to initialize command slab");
		return -1;
	}
	queue_init(&pd->cmd_queue);
	return 0;
}

static struct osdp_cmd *cp_cmd_alloc(struct osdp_pd *pd)
{
	struct cp_cmd_node *n = NULL;

	if (slab_alloc(&pd->app_data.slab, (void **)&n)) {
		LOG_ERR("Command slab allocation failed");
		return NULL;
	}
	memset(&n->object, 0, sizeof(n->object));
	return &n->object;
}

static void cp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct cp_cmd_node *n;

	n = CONTAINER_OF(cmd, struct cp_cmd_node, object);
	slab_free(&pd->app_data.slab, n);
}

static void cp_cmd_enqueue(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct cp_cmd_node *n;

	n = CONTAINER_OF(cmd, struct cp_cmd_node, object);
	queue_enqueue(&pd->cmd_queue, &n->node);
}

static int cp_cmd_dequeue(struct osdp_pd *pd, struct osdp_cmd **cmd)
{
	struct cp_cmd_node *n;
	queue_node_t *node;

	if (queue_dequeue(&pd->cmd_queue, &node)) {
		return -1;
	}
	n = CONTAINER_OF(node, struct cp_cmd_node, node);
	*cmd = &n->object;
	return 0;
}

static int cp_channel_acquire(struct osdp_pd *pd, int *owner)
{
	int i;
	struct osdp *ctx = pd_to_osdp(pd);

	if (ctx->channel_lock[pd->idx] == pd->channel.id) {
		return 0; /* already acquired! by current PD */
	}
	assert(ctx->channel_lock[pd->idx] == 0);
	for (i = 0; i < NUM_PD(ctx); i++) {
		if (ctx->channel_lock[i] == pd->channel.id) {
			/* some other PD has locked this channel */
			if (owner != NULL) {
				*owner = i;
			}
			return -1;
		}
	}
	ctx->channel_lock[pd->idx] = pd->channel.id;

	return 0;
}

static int cp_channel_release(struct osdp_pd *pd)
{
	struct osdp *ctx = pd_to_osdp(pd);

	if (ctx->channel_lock[pd->idx] != pd->channel.id) {
		LOG_ERR("Attempt to release another PD's channel lock");
		return -1;
	}
	ctx->channel_lock[pd->idx] = 0;

	return 0;
}

static const char *cp_get_cap_name(int cap)
{
	if (cap <= OSDP_PD_CAP_UNUSED || cap >= OSDP_PD_CAP_SENTINEL) {
		return NULL;
	}
	const char *cap_name[] = {
		[OSDP_PD_CAP_CONTACT_STATUS_MONITORING] = "ContactStatusMonitoring",
		[OSDP_PD_CAP_OUTPUT_CONTROL] = "OutputControl",
		[OSDP_PD_CAP_CARD_DATA_FORMAT] = "CardDataFormat",
		[OSDP_PD_CAP_READER_LED_CONTROL] = "LEDControl",
		[OSDP_PD_CAP_READER_AUDIBLE_OUTPUT] = "AudibleControl",
		[OSDP_PD_CAP_READER_TEXT_OUTPUT] = "TextOutput",
		[OSDP_PD_CAP_TIME_KEEPING] = "TimeKeeping",
		[OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT] = "CheckCharacter",
		[OSDP_PD_CAP_COMMUNICATION_SECURITY] = "CommunicationSecurity",
		[OSDP_PD_CAP_RECEIVE_BUFFERSIZE] = "ReceiveBufferSize",
		[OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE] = "CombinedMessageSize",
		[OSDP_PD_CAP_SMART_CARD_SUPPORT] = "SmartCard",
		[OSDP_PD_CAP_READERS] = "Reader",
		[OSDP_PD_CAP_BIOMETRICS] = "Biometric",
		[OSDP_PD_CAP_SECURE_PIN_ENTRY] = "SecurePinEntry",
		[OSDP_PD_CAP_OSDP_VERSION] = "OsdpVersion",
	};
	return cap_name[cap];
}

static inline void assert_buf_len(int need, int have)
{
	__ASSERT(need < have, "OOM at build command: need:%d have:%d",
		 need, have);
}

static int cp_build_command(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	struct osdp_cmd *cmd = NULL;
	int ret, len = 0;
	int data_off = osdp_phy_packet_get_data_offset(pd, buf);
	uint8_t *smb = osdp_phy_packet_get_smb(pd, buf);

	buf += data_off;
	max_len -= data_off;
	if (max_len <= 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
		assert_buf_len(CMD_POLL_LEN, max_len);
		buf[len++] = pd->cmd_id;
		break;
	case CMD_LSTAT:
		assert_buf_len(CMD_LSTAT_LEN, max_len);
		buf[len++] = pd->cmd_id;
		break;
	case CMD_ISTAT:
		assert_buf_len(CMD_ISTAT_LEN, max_len);
		buf[len++] = pd->cmd_id;
		break;
	case CMD_OSTAT:
		assert_buf_len(CMD_OSTAT_LEN, max_len);
		buf[len++] = pd->cmd_id;
		break;
	case CMD_RSTAT:
		assert_buf_len(CMD_RSTAT_LEN, max_len);
		buf[len++] = pd->cmd_id;
		break;
	case CMD_ID:
		assert_buf_len(CMD_ID_LEN, max_len);
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		break;
	case CMD_CAP:
		assert_buf_len(CMD_CAP_LEN, max_len);
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		break;
	case CMD_OUT:
		assert_buf_len(CMD_OUT_LEN, max_len);
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.timer_count);
		buf[len++] = BYTE_1(cmd->output.timer_count);
		break;
	case CMD_LED:
		assert_buf_len(CMD_LED_LEN, max_len);
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
		break;
	case CMD_BUZ:
		assert_buf_len(CMD_BUZ_LEN, max_len);
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.control_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		break;
	case CMD_TEXT:
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		assert_buf_len(CMD_TEXT_LEN + cmd->text.length, max_len);
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.control_code;
		buf[len++] = cmd->text.temp_time;
		buf[len++] = cmd->text.offset_row;
		buf[len++] = cmd->text.offset_col;
		buf[len++] = cmd->text.length;
		memcpy(buf + len, cmd->text.data, cmd->text.length);
		len += cmd->text.length;
		break;
	case CMD_COMSET:
		assert_buf_len(CMD_COMSET_LEN, max_len);
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->comset.address;
		buf[len++] = BYTE_0(cmd->comset.baud_rate);
		buf[len++] = BYTE_1(cmd->comset.baud_rate);
		buf[len++] = BYTE_2(cmd->comset.baud_rate);
		buf[len++] = BYTE_3(cmd->comset.baud_rate);
		break;
	case CMD_MFG:
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		assert_buf_len(CMD_MFG_LEN + cmd->mfg.length, max_len);
		if (cmd->mfg.length > OSDP_CMD_MFG_MAX_DATALEN) {
			LOG_ERR("Invalid MFG data length (%d)", cmd->mfg.length);
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(cmd->mfg.vendor_code);
		buf[len++] = BYTE_1(cmd->mfg.vendor_code);
		buf[len++] = BYTE_2(cmd->mfg.vendor_code);
		memcpy(buf + len, cmd->mfg.data, cmd->mfg.length);
		len += cmd->mfg.length;
		break;
	case CMD_ACURXSIZE:
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(OSDP_PACKET_BUF_SIZE);
		buf[len++] = BYTE_1(OSDP_PACKET_BUF_SIZE);
		break;
	case CMD_KEEPACTIVE:
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(0); // keepalive in ms time LSB
		buf[len++] = BYTE_1(0); // keepalive in ms time MSB
		break;
	case CMD_ABORT:
		buf[len++] = pd->cmd_id;
		break;
	case CMD_FILETRANSFER:
		ret = osdp_file_cmd_tx_build(pd, buf + len + 1, max_len);
		if (ret <= 0) {
			/* (Only) Abort file transfer on failures */
			buf[len++] = CMD_ABORT;
			break;
		}
		buf[len++] = pd->cmd_id;
		len += ret;
		break;
	case CMD_KEYSET:
		if (!sc_is_active(pd)) {
			LOG_ERR("Cannot perform KEYSET without SC!");
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		assert_buf_len(CMD_KEYSET_LEN, max_len);
		if (cmd->keyset.length != 16) {
			LOG_ERR("Invalid key length");
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 1;  /* key type (1: SCBK) */
		buf[len++] = 16; /* key length in bytes */
		if (cmd->keyset.type == 1) { /* SCBK */
			memcpy(buf + len, cmd->keyset.data, 16);
		} else if (cmd->keyset.type == 0) {  /* master_key */
			osdp_compute_scbk(pd, cmd->keyset.data, buf + len);
		} else {
			LOG_ERR("Unknown key type (%d)", cmd->keyset.type);
			return -1;
		}
		len += 16;
		break;
	case CMD_CHLNG:
		assert_buf_len(CMD_CHLNG_LEN, max_len);
		if (smb == NULL) {
			LOG_ERR("Invalid secure message block!");
			return OSDP_CP_ERR_GENERIC;
		}
		smb[0] = 3;       /* length */
		smb[1] = SCS_11;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		memcpy(buf + len, pd->sc.cp_random, 8);
		len += 8;
		break;
	case CMD_SCRYPT:
		assert_buf_len(CMD_SCRYPT_LEN, max_len);
		if (smb == NULL) {
			LOG_ERR("Invalid secure message block!");
			return OSDP_CP_ERR_GENERIC;
		}
		osdp_compute_cp_cryptogram(pd);
		smb[0] = 3;       /* length */
		smb[1] = SCS_13;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		memcpy(buf + len, pd->sc.cp_cryptogram, 16);
		len += 16;
		break;
	default:
		LOG_ERR("Unknown/Unsupported CMD: %s(%02x)",
			osdp_cmd_name(pd->cmd_id), pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (smb && (smb[1] > SCS_14) && sc_is_active(pd)) {
		/**
		 * When SC active and current cmd is not a handshake (<= SCS_14)
		 * then we must set SCS type to 17 if this message has data
		 * bytes and 15 otherwise.
		 */
		smb[0] = 2;
		smb[1] = (len > 1) ? SCS_17 : SCS_15;
	}

	return len;
}

static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	int i, ret = OSDP_CP_ERR_GENERIC, pos = 0, t1, t2;
	struct osdp_event event;

	pd->reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	switch (pd->reply_id) {
	case REPLY_ACK:
		if (len != REPLY_ACK_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_NAK:
		if (len != REPLY_NAK_DATA_LEN) {
			break;
		}
		if (buf[pos] == OSDP_PD_NAK_MSG_CHK &&
		    ISSET_FLAG(pd, PD_FLAG_CP_USE_CRC)) {
			LOG_INF("PD NAK'd CRC-16, falling back to checksum");
			CLEAR_FLAG(pd, PD_FLAG_CP_USE_CRC);
			ret = OSDP_CP_ERR_RETRY_CMD;
			break;
		}
		LOG_WRN("PD replied with NAK(%d) for CMD: %s(%02x)",
			buf[pos], osdp_cmd_name(pd->cmd_id), pd->cmd_id);
		ret = OSDP_CP_ERR_NONE;
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

		pd->id.serial_number = buf[pos++];
		pd->id.serial_number |= buf[pos++] << 8;
		pd->id.serial_number |= buf[pos++] << 16;
		pd->id.serial_number |= buf[pos++] << 24;

		pd->id.firmware_version = buf[pos++] << 16;
		pd->id.firmware_version |= buf[pos++] << 8;
		pd->id.firmware_version |= buf[pos++];
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_PDCAP:
		if ((len % REPLY_PDCAP_ENTITY_LEN) != 0) {
			LOG_ERR("PDCAP response length is not a multiple of 3");
			return OSDP_CP_ERR_GENERIC;
		}
		while (pos < len) {
			t1 = buf[pos++]; /* func_code */
			if (t1 >= OSDP_PD_CAP_SENTINEL) {
				break;
			}
			pd->cap[t1].function_code = t1;
			pd->cap[t1].compliance_level = buf[pos++];
			pd->cap[t1].num_items = buf[pos++];
			LOG_DBG("Reports capability '%s' (%d/%d)",
				cp_get_cap_name(pd->cap[t1].function_code),
				pd->cap[t1].compliance_level,
				pd->cap[t1].num_items);
		}

		/* Get peer RX buffer size */
		t1 = OSDP_PD_CAP_RECEIVE_BUFFERSIZE;
		if (pd->cap[t1].function_code == t1) {
			pd->peer_rx_size = pd->cap[t1].compliance_level;
			pd->peer_rx_size |= pd->cap[t1].num_items << 8;
		}

		/* post-capabilities hooks */
		t2 = OSDP_PD_CAP_COMMUNICATION_SECURITY;
		if (pd->cap[t2].compliance_level & 0x01) {
			SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_SC_CAPABLE);
		}

		/* Check checksum/CRC support capability */
		t1 = OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT;
		if (pd->cap[t1].function_code == t1) {
			if (pd->cap[t1].compliance_level & 0x01) {
				SET_FLAG(pd, PD_FLAG_CP_USE_CRC);
			} else {
				CLEAR_FLAG(pd, PD_FLAG_CP_USE_CRC);
			}
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_OSTATR: {
		int cap_num = OSDP_PD_CAP_OUTPUT_CONTROL;

		if (len != pd->cap[cap_num].num_items ||
		    len > OSDP_STATUS_REPORT_MAX_LEN) {
			LOG_ERR("Invalid output status report length %d", len);
			return OSDP_CP_ERR_GENERIC;
		}
		event.type = OSDP_EVENT_STATUS;
		event.status.type = OSDP_STATUS_REPORT_OUTPUT;
		event.status.nr_entries = len;
		for (i = 0; i < len; i++) {
			event.status.report[i] = buf[pos++];
		}
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	}
	case REPLY_ISTATR: {
		int cap_num = OSDP_PD_CAP_CONTACT_STATUS_MONITORING;

		if (len != pd->cap[cap_num].num_items ||
		    len > OSDP_STATUS_REPORT_MAX_LEN) {
			LOG_ERR("Invalid input status report length %d", len);
			return OSDP_CP_ERR_GENERIC;
		}
		event.type = OSDP_EVENT_STATUS;
		event.status.type = OSDP_STATUS_REPORT_INPUT;
		event.status.nr_entries = len;
		for (i = 0; i < len; i++) {
			event.status.report[i] = buf[pos++];
		}
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	}
	case REPLY_LSTATR:
		if (len != REPLY_LSTATR_DATA_LEN) {
			break;
		}
		event.type = OSDP_EVENT_STATUS;
		event.status.type = OSDP_STATUS_REPORT_LOCAL;
		event.status.nr_entries = 2;
		event.status.report[0] = buf[pos++];
		event.status.report[1] = buf[pos++];
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RSTATR:
		if (len != REPLY_RSTATR_DATA_LEN) {
			break;
		}
		event.type = OSDP_EVENT_STATUS;
		event.status.type = OSDP_STATUS_REPORT_REMOTE;
		event.status.nr_entries = 1;
		event.status.report[0] = buf[pos++];
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_COM:
		if (len != REPLY_COM_DATA_LEN) {
			break;
		}
		t1 = buf[pos++];
		temp32 = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_INF("COMSET responded with ID:%d Baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_KEYPAD:
		if (len < REPLY_KEYPAD_DATA_LEN) {
			break;
		}
		event.type = OSDP_EVENT_KEYPRESS;
		event.keypress.reader_no = buf[pos++];
		event.keypress.length = buf[pos++];
		if ((len - REPLY_KEYPAD_DATA_LEN) != event.keypress.length) {
			break;
		}
		memcpy(event.keypress.data, buf + pos, event.keypress.length);
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.format = buf[pos++];
		event.cardread.length = buf[pos++]; /* bits LSB */
		event.cardread.length |= buf[pos++] << 8; /* bits MSB */
		event.cardread.direction = 0; /* un-specified */
		t1 = (event.cardread.length + 7) / 8; /* len: bytes */
		if (t1 != (len - REPLY_RAW_DATA_LEN)) {
			break;
		}
		memcpy(event.cardread.data, buf + pos, t1);
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_FMT:
		/**
		 * osdp_FMT was underspecified by SIA from get-go. It was marked
		 * for deprecation in v2.2.2. To avoid confusions, we will just
		 * ignore it here.
		 *
		 * See: https://github.com/goToMain/libosdp/issues/206
		 */
		LOG_WRN("Ignoring deprecated response osdp_FMT");
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		if (len != REPLY_BUSY_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_RETRY_CMD;
		break;
	case REPLY_MFGREP:
		if (len < REPLY_MFGREP_LEN) {
			break;
		}
		event.type = OSDP_EVENT_MFGREP;
		event.mfgrep.vendor_code = buf[pos++];
		event.mfgrep.vendor_code |= buf[pos++] << 8;
		event.mfgrep.vendor_code |= buf[pos++] << 16;
		event.mfgrep.length = len - REPLY_MFGREP_LEN;
		if (event.mfgrep.length > OSDP_EVENT_MFGREP_MAX_DATALEN) {
			break;
		}
		memcpy(event.mfgrep.data, buf + pos, event.mfgrep.length);
		memcpy(pd->ephemeral_data, &event, sizeof(event));
		make_request(pd, CP_REQ_EVENT_SEND);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_FTSTAT:
		ret = osdp_file_cmd_stat_decode(pd, buf + pos, len);
		break;
	case REPLY_CCRYPT:
		if (sc_is_active(pd) || pd->cmd_id != CMD_CHLNG) {
			LOG_EM("Out of order REPLY_CCRYPT; has PD gone rogue?");
			break;
		}
		if (len != REPLY_CCRYPT_DATA_LEN) {
			break;
		}
		memcpy(pd->sc.pd_client_uid, buf + pos, 8);
		memcpy(pd->sc.pd_random, buf + pos + 8, 8);
		memcpy(pd->sc.pd_cryptogram, buf + pos + 16, 16);
		pos += 32;
		osdp_compute_session_keys(pd);
		if (osdp_verify_pd_cryptogram(pd) != 0) {
			LOG_ERR("Failed to verify PD cryptogram");
			return OSDP_CP_ERR_APP;
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RMAC_I:
		if (sc_is_active(pd) || pd->cmd_id != CMD_SCRYPT) {
			LOG_EM("Out of order REPLY_RMAC_I; has PD gone rogue?");
			break;
		}
		if (len != REPLY_RMAC_I_DATA_LEN) {
			break;
		}
		memcpy(pd->sc.r_mac, buf + pos, 16);
		ret = OSDP_CP_ERR_NONE;
		break;
	default:
		LOG_WRN("Unknown reply %s(%02x)",
			osdp_reply_name(pd->reply_id), pd->reply_id);
		return OSDP_CP_ERR_UNKNOWN;
	}

	if (ret != OSDP_CP_ERR_NONE) {
		LOG_ERR("Failed to decode REPLY: %s(%02x) for CMD: %s(%02x)",
			osdp_reply_name(pd->reply_id), pd->reply_id,
			osdp_cmd_name(pd->cmd_id), pd->cmd_id);
	}

	if (pd->cmd_id != CMD_POLL ||
	    (pd->cmd_id == CMD_POLL && pd->reply_id != REPLY_ACK)) {
		LOG_DBG("CMD: %s(%02x) REPLY: %s(%02x)",
			osdp_cmd_name(pd->cmd_id), pd->cmd_id,
			osdp_reply_name(pd->reply_id), pd->reply_id);
	}

	return ret;
}

static void do_event_callback(struct osdp_pd *pd)
{
	struct osdp *ctx = pd_to_osdp(pd);

	if (ctx->event_callback) {
		ctx->event_callback(ctx->event_callback_arg, pd->idx,
				    (struct osdp_event *)pd->ephemeral_data);
	}
}

static int cp_build_and_send_packet(struct osdp_pd *pd)
{
	int ret, packet_buf_size = get_tx_buf_size(pd);

	/* init packet buf with header */
	ret = osdp_phy_packet_init(pd, pd->packet_buf, packet_buf_size);
	if (ret < 0) {
		return OSDP_CP_ERR_GENERIC;
	}
	pd->packet_buf_len = ret;

	/* fill command data */
	ret = cp_build_command(pd, pd->packet_buf, packet_buf_size);
	if (ret < 0) {
		return OSDP_CP_ERR_GENERIC;
	}
	pd->packet_buf_len += ret;

	ret = osdp_phy_send_packet(pd, pd->packet_buf, pd->packet_buf_len,
				   packet_buf_size);
	if (ret < 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	return OSDP_CP_ERR_NONE;
}

static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int err, len;

	err = osdp_phy_check_packet(pd);

	/* Translate phy error codes to CP errors */
	switch (err) {
	case OSDP_ERR_PKT_NONE:
		break;
	case OSDP_ERR_PKT_WAIT:
	case OSDP_ERR_PKT_NO_DATA:
		return OSDP_CP_ERR_NO_DATA;
	case OSDP_ERR_PKT_BUSY:
		return OSDP_CP_ERR_RETRY_CMD;
	case OSDP_ERR_PKT_NACK:
		if (pd->ephemeral_data[0] == OSDP_PD_NAK_SEQ_NUM) {
			LOG_WRN("NAK(SEQ_NUM); restarting communication");
			osdp_phy_state_reset(pd, true);
			sc_deactivate(pd);
			pd->state = OSDP_CP_STATE_INIT;
			return OSDP_CP_ERR_SEQ_NUM;
		}
		/* Other NACKs: CP cannot do anything about an invalid reply from a PD.
		 * Default to going offline and retrying after a while. The reason for
		 * this failure was probably better logged by lower layers.
		 */
		__fallthrough;
	default:
		return OSDP_CP_ERR_GENERIC;
	}

	/* Valid OSDP packet in buffer */
	len = osdp_phy_decode_packet(pd, &buf);
	if (len <= 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	return cp_decode_response(pd, buf, len);
}

static inline bool cp_sc_should_retry(struct osdp_pd *pd)
{
	return (sc_is_capable(pd) && !sc_is_active(pd) &&
		osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_MS);
}

static int cp_translate_cmd(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	/* Make a local copy of osdp_cmd command to be used later */
	memcpy(pd->ephemeral_data, cmd, sizeof(struct osdp_cmd));
	cmd = (struct osdp_cmd *)pd->ephemeral_data;

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT: return CMD_OUT;
	case OSDP_CMD_LED:    return CMD_LED;
	case OSDP_CMD_BUZZER: return CMD_BUZ;
	case OSDP_CMD_TEXT:   return CMD_TEXT;
	case OSDP_CMD_COMSET: return CMD_COMSET;
	case OSDP_CMD_MFG:    return CMD_MFG;
	case OSDP_CMD_STATUS:
		switch (cmd->status.type) {
		case OSDP_STATUS_REPORT_INPUT:  return CMD_ISTAT;
		case OSDP_STATUS_REPORT_OUTPUT: return CMD_OSTAT;
		case OSDP_STATUS_REPORT_LOCAL:  return CMD_LSTAT;
		case OSDP_STATUS_REPORT_REMOTE: return CMD_RSTAT;
		default: return -1;
		}
	case OSDP_CMD_KEYSET:
		if (cmd->keyset.type != 1 || !sc_is_active(pd)) {
			return -1;
		} else {
			return CMD_KEYSET;
		}
	case OSDP_CMD_FILE_TX:
		/**
		 * This external command is handled as multiple command from
		 * osdp_file.c and it maintains it's own state. This means we
		 * should never reach here unless something is wrong.
		 */
		__fallthrough;
	default: BUG();
	}

	return -1;
}

static void fill_local_keyset_cmd(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd = (struct osdp_cmd *)pd->ephemeral_data;

	cmd->id = OSDP_CMD_KEYSET;
	cmd->keyset.type = 1;
	cmd->keyset.length = sizeof(pd->sc.scbk);
	memcpy(cmd->keyset.data, pd->sc.scbk, sizeof(pd->sc.scbk));
}

static inline bool cp_phy_running(struct osdp_pd *pd)
{
	return (pd->phy_state == OSDP_CP_PHY_STATE_SEND_CMD ||
		pd->phy_state == OSDP_CP_PHY_STATE_REPLY_WAIT ||
		pd->phy_state == OSDP_CP_PHY_STATE_WAIT);
}

static inline bool cp_phy_kick(struct osdp_pd *pd)
{
	if (pd->phy_state == OSDP_CP_PHY_STATE_IDLE) {
		pd->phy_state = OSDP_CP_PHY_STATE_SEND_CMD;
		return true;
	}
	return false;
}

static void cp_phy_state_done(struct osdp_pd *pd)
{
	/* called when we have a valid response from the PD */
	if (sc_is_active(pd)) {
		pd->sc_tstamp = osdp_millis_now();
	}
	pd->phy_retry_count = 0;
	pd->phy_state = OSDP_CP_PHY_STATE_DONE;
}

static void cp_phy_state_wait(struct osdp_pd *pd, uint32_t wait_ms)
{
	pd->wait_ms = wait_ms;
	pd->phy_tstamp = osdp_millis_now();
	pd->phy_state = OSDP_CP_PHY_STATE_WAIT;
}

static int cp_phy_state_update(struct osdp_pd *pd)
{
	int rc, ret = OSDP_CP_ERR_CAN_YIELD;

	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_DONE:
	case OSDP_CP_PHY_STATE_IDLE:
		ret = OSDP_CP_ERR_NONE;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_WAIT:
		if (osdp_millis_since(pd->phy_tstamp) < pd->wait_ms) {
			return OSDP_CP_ERR_CAN_YIELD;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_SEND_CMD;
		__fallthrough;
	case OSDP_CP_PHY_STATE_SEND_CMD:
		/* Check if we have any commands in the queue */
		if (cp_build_and_send_packet(pd)) {
			LOG_ERR("Failed to build/send packet for CMD: %s(%02x)",
				osdp_cmd_name(pd->cmd_id), pd->cmd_id);
			goto error;
		}
		ret = OSDP_CP_ERR_INPROG;
		osdp_phy_state_reset(pd, false);
		pd->reply_id = REPLY_INVALID;
		pd->phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		rc = cp_process_reply(pd);
		if (rc == OSDP_CP_ERR_NONE || rc == OSDP_CP_ERR_APP) {
			pd->tstamp = osdp_millis_now();
			osdp_phy_progress_sequence(pd);
		}
		if (rc == OSDP_CP_ERR_NONE ||
		    rc == OSDP_CP_ERR_SEQ_NUM ||
		    (rc == OSDP_CP_ERR_UNKNOWN && pd->cmd_id == CMD_POLL &&
		     ISSET_FLAG(pd, OSDP_FLAG_IGN_UNSOLICITED))) {
			cp_phy_state_done(pd);
			return OSDP_CP_ERR_NONE;
		}
		if (rc == OSDP_CP_ERR_GENERIC ||
		    rc == OSDP_CP_ERR_UNKNOWN ||
		    rc == OSDP_CP_ERR_APP) {
			goto error;
		}
		if (rc == OSDP_CP_ERR_RETRY_CMD) {
			cp_phy_state_wait(pd, OSDP_CMD_RETRY_WAIT_MS);
			return OSDP_CP_ERR_CAN_YIELD;
		}
		if (osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			if (pd->phy_retry_count < OSDP_CMD_MAX_RETRIES) {
				pd->phy_retry_count += 1;
				LOG_WRN("No response in 200ms; probing (%d)",
					pd->phy_retry_count);
				cp_phy_state_wait(pd, OSDP_CMD_RETRY_WAIT_MS);
				return OSDP_CP_ERR_CAN_YIELD;
			}
			LOG_ERR("Response timeout for CMD: %s(%02x)",
				osdp_cmd_name(pd->cmd_id), pd->cmd_id);
			goto error;
		}
		ret = OSDP_CP_ERR_INPROG;
		break;
	}

	return ret;
error:
	pd->phy_state = OSDP_CP_PHY_STATE_ERR;
	return OSDP_CP_ERR_GENERIC;
}

static const char *state_get_name(enum osdp_cp_state_e state)
{
	switch (state) {
	case OSDP_CP_STATE_INIT:      return "ID-Request";
	case OSDP_CP_STATE_CAPDET:    return "Cap-Detect";
	case OSDP_CP_STATE_SC_CHLNG:  return "SC-Chlng";
	case OSDP_CP_STATE_SC_SCRYPT: return "SC-Scrypt";
	case OSDP_CP_STATE_SET_SCBK:  return "SC-SetSCBK";
	case OSDP_CP_STATE_ONLINE:    return "Online";
	case OSDP_CP_STATE_OFFLINE:   return "Offline";
	case OSDP_CP_STATE_DISABLED:  return "Disabled";
	default:
		BUG();
	}
}

static int cp_get_online_command(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;
	int ret;

	if (cp_cmd_dequeue(pd, &cmd) == 0) {
		ret = cp_translate_cmd(pd, cmd);
		if (cmd->flags & OSDP_CMD_FLAG_BROADCAST) {
			SET_FLAG(pd, PD_FLAG_PKT_BROADCAST);
		}
		cp_cmd_free(pd, cmd);
		return ret;
	}

	ret = osdp_file_tx_get_command(pd);
	if (ret != 0) {
		return ret;
	}

	if (osdp_millis_since(pd->tstamp) > OSDP_PD_POLL_TIMEOUT_MS) {
		pd->tstamp = osdp_millis_now();
		return CMD_POLL;
	}

	return -1;
}

static void notify_pd_status(struct osdp_pd *pd, bool is_online)
{
	struct osdp *ctx = pd_to_osdp(pd);
	struct osdp_event evt;

	if (!ctx->event_callback ||
	    !ISSET_FLAG(pd, OSDP_FLAG_ENABLE_NOTIFICATION)) {
		return;
	}

	evt.type = OSDP_EVENT_NOTIFICATION;
	evt.notif.type = OSDP_EVENT_NOTIFICATION_PD_STATUS;
	evt.notif.arg0 = is_online;
	ctx->event_callback(ctx->event_callback_arg, pd->idx, &evt);
}

static void notify_sc_status(struct osdp_pd *pd)
{
	struct osdp *ctx = pd_to_osdp(pd);
	struct osdp_event evt;

	if (!ctx->event_callback ||
	    !ISSET_FLAG(pd, OSDP_FLAG_ENABLE_NOTIFICATION)) {
		return;
	}

	evt.type = OSDP_EVENT_NOTIFICATION;
	evt.notif.type = OSDP_EVENT_NOTIFICATION_SC_STATUS;
	evt.notif.arg0 = sc_is_active(pd);
	evt.notif.arg1 = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
	ctx->event_callback(ctx->event_callback_arg, pd->idx, &evt);
}

static void cp_keyset_complete(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	if (!ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		memcpy(pd->sc.scbk, cmd->keyset.data, 16);
	} else {
		CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
	}
	sc_deactivate(pd);
	notify_sc_status(pd);
	if (pd->state == OSDP_CP_STATE_ONLINE) {
		make_request(pd, CP_REQ_RESTART_SC);
		LOG_INF("SCBK set; restarting SC to verify new SCBK");
	}
}

static bool cp_check_online_response(struct osdp_pd *pd)
{
	/* Always allow an ACK from the PD; Also, the most common case */
	if (pd->reply_id == REPLY_ACK) {
		if (pd->cmd_id == CMD_KEYSET) {
			/**
			 * When we received an ACK for keyset (either in current
			 * SC session or in plaintext after PD discarded the SC
			 * in favour of the new SCBK), we need to call to commit
			 * the new key pd->sc.scbk and restart the SC.
			 */
			cp_keyset_complete(pd);
		}
		return true;
	}

	/* A NAK or no response is always an error */
	if (pd->reply_id == REPLY_NAK || pd->reply_id == REPLY_INVALID) {
		return false;
	}

	/* Check for known poll responses */
	if (pd->cmd_id == CMD_POLL) {
		if (pd->reply_id == REPLY_LSTATR ||
		    pd->reply_id == REPLY_ISTATR ||
		    pd->reply_id == REPLY_OSTATR ||
		    pd->reply_id == REPLY_RSTATR ||
		    pd->reply_id == REPLY_MFGREP ||
		    pd->reply_id == REPLY_RAW ||
		    pd->reply_id == REPLY_KEYPAD) {
			return true;
		}
		return ISSET_FLAG(pd, OSDP_FLAG_IGN_UNSOLICITED);
	}

	/* Otherwise, we permit only expected responses */
	switch (pd->cmd_id) {
	case CMD_FILETRANSFER: return pd->reply_id == REPLY_FTSTAT;
	case CMD_COMSET:       return pd->reply_id == REPLY_COM;
	case CMD_MFG:          return pd->reply_id == REPLY_MFGREP;
	case CMD_LSTAT:        return pd->reply_id == REPLY_LSTATR;
	case CMD_ISTAT:        return pd->reply_id == REPLY_ISTATR;
	case CMD_OSTAT:        return pd->reply_id == REPLY_OSTATR;
	case CMD_RSTAT:        return pd->reply_id == REPLY_RSTATR;
	default:
		LOG_ERR("Unexpected respose: CMD: %s(%02x) REPLY: %s(%02x)",
			osdp_cmd_name(pd->cmd_id), pd->cmd_id,
			osdp_reply_name(pd->reply_id), pd->reply_id);
		return false;
	}
}

static inline int state_get_cmd(struct osdp_pd *pd)
{
	enum osdp_cp_state_e state = pd->state;

	switch (state) {
	case OSDP_CP_STATE_INIT:      return CMD_ID;
	case OSDP_CP_STATE_CAPDET:    return CMD_CAP;
	case OSDP_CP_STATE_SC_CHLNG:  return CMD_CHLNG;
	case OSDP_CP_STATE_SC_SCRYPT: return CMD_SCRYPT;
	case OSDP_CP_STATE_SET_SCBK:  return CMD_KEYSET;
	case OSDP_CP_STATE_ONLINE:    return cp_get_online_command(pd);
	default: return -1;
	}
}

static inline bool state_check_reply(struct osdp_pd *pd)
{
	enum osdp_cp_state_e state = pd->state;

	switch (state) {
	case OSDP_CP_STATE_INIT:      return pd->reply_id == REPLY_PDID;
	case OSDP_CP_STATE_CAPDET:    return pd->reply_id == REPLY_PDCAP;
	case OSDP_CP_STATE_SC_CHLNG:  return pd->reply_id == REPLY_CCRYPT;
	case OSDP_CP_STATE_SC_SCRYPT: return pd->reply_id == REPLY_RMAC_I;
	case OSDP_CP_STATE_SET_SCBK:  return pd->reply_id == REPLY_ACK;
	case OSDP_CP_STATE_ONLINE:    return cp_check_online_response(pd);
	default: return false;
	}
}

static enum osdp_cp_state_e get_next_ok_state(struct osdp_pd *pd)
{
	enum osdp_cp_state_e state = pd->state;

	switch (state) {
	case OSDP_CP_STATE_INIT:
		return OSDP_CP_STATE_CAPDET;
	case OSDP_CP_STATE_CAPDET:
		if (sc_is_capable(pd)) {
			CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			return OSDP_CP_STATE_SC_CHLNG;
		}
		if (is_enforce_secure(pd)) {
			LOG_INF("SC disabled/incapable; Set PD offline "
				"due to ENFORCE_SECURE");
			return OSDP_CP_STATE_OFFLINE;
		}
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_SC_CHLNG:
		return OSDP_CP_STATE_SC_SCRYPT;
	case OSDP_CP_STATE_SC_SCRYPT:
		sc_activate(pd);
		notify_sc_status(pd);
		if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_WRN("SC active with SCBK-D. Set SCBK");
			fill_local_keyset_cmd(pd);
			return OSDP_CP_STATE_SET_SCBK;
		}
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_SET_SCBK:
		cp_keyset_complete(pd);
		return OSDP_CP_STATE_SC_CHLNG;
	case OSDP_CP_STATE_ONLINE:
		if (cp_sc_should_retry(pd)) {
			LOG_INF("Attempting to restart SC after %d seconds",
				OSDP_PD_SC_RETRY_MS/1000);
			return OSDP_CP_STATE_SC_CHLNG;
		}
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_OFFLINE:
		if (osdp_millis_since(pd->tstamp) > pd->wait_ms) {
			return OSDP_CP_STATE_INIT;
		}
		return OSDP_CP_STATE_OFFLINE;
	case OSDP_CP_STATE_DISABLED:
		return OSDP_CP_STATE_DISABLED;
	default: BUG();
	}
}

static enum osdp_cp_state_e get_next_err_state(struct osdp_pd *pd)
{
	enum osdp_cp_state_e state = pd->state;

	switch (state) {
	case OSDP_CP_STATE_INIT:
		return OSDP_CP_STATE_OFFLINE;
	case OSDP_CP_STATE_CAPDET:
		return OSDP_CP_STATE_OFFLINE;
	case OSDP_CP_STATE_SC_CHLNG:
		if (is_enforce_secure(pd)) {
			LOG_ERR("CHLNG failed. Set PD offline due to "
				"ENFORCE_SECURE");
			return OSDP_CP_STATE_OFFLINE;
		}
		if (!ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			LOG_WRN("SC Failed. Retry with SCBK-D");
			return OSDP_CP_STATE_SC_CHLNG;
		}
		CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
		/**
		 * SC setup failed; Update sc_tstamp so the next retry happens
		 * after OSDP_PD_SC_RETRY_MS.
		 */
		pd->sc_tstamp = osdp_millis_now();
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_SC_SCRYPT:
		if (is_enforce_secure(pd)) {
			LOG_ERR("SCRYPT failed. Set PD offline due to "
				"ENFORCE_SECURE");
			return OSDP_CP_STATE_OFFLINE;
		}
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_SET_SCBK:
		sc_deactivate(pd);
		notify_sc_status(pd);
		if (is_enforce_secure(pd) ||
		    ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_ERR("Failed to set SCBK; "
				"Set PD offline due to ENFORCE_SECURE");
			return OSDP_CP_STATE_OFFLINE;
		}
		return OSDP_CP_STATE_ONLINE;
	case OSDP_CP_STATE_ONLINE:
		return OSDP_CP_STATE_OFFLINE;
	case OSDP_CP_STATE_OFFLINE:
		return OSDP_CP_STATE_OFFLINE;
	case OSDP_CP_STATE_DISABLED:
		return OSDP_CP_STATE_DISABLED;
	default: BUG();
	}
}

static inline enum osdp_cp_state_e get_next_state(struct osdp_pd *pd, int err)
{
	return (err == 0) ? get_next_ok_state(pd) : get_next_err_state(pd);
}

static void cp_state_change(struct osdp_pd *pd, enum osdp_cp_state_e next)
{
	enum osdp_cp_state_e cur = pd->state;

	switch (next) {
	case OSDP_CP_STATE_INIT:
		osdp_phy_state_reset(pd, true);
		break;
	case OSDP_CP_STATE_ONLINE:
		LOG_INF("Online; %s SC", sc_is_active(pd) ? "With" : "Without");
		notify_pd_status(pd, true);
		break;
	case OSDP_CP_STATE_OFFLINE:
		pd->tstamp = osdp_millis_now();
		pd->wait_ms = OSDP_ONLINE_RETRY_WAIT_MAX_MS;
		sc_deactivate(pd);
		notify_sc_status(pd);
		LOG_ERR("Going offline for %d seconds; Was in '%s' state",
			pd->wait_ms / 1000, state_get_name(cur));
		notify_pd_status(pd, false);
		break;
	case OSDP_CP_STATE_SC_CHLNG:
		osdp_sc_setup(pd);
		break;
	case OSDP_CP_STATE_DISABLED:
		sc_deactivate(pd);
		notify_sc_status(pd);
		notify_pd_status(pd, false);
		osdp_phy_state_reset(pd, true);
		LOG_INF("PD disabled; going offline until re-enabled");
		break;
	default: break;
	}

	LOG_DBG("StateChange: [%s] -> [%s] (SC-%s%s)",
		state_get_name(cur),
		state_get_name(next),
		sc_is_active(pd) ? "Active" : "Inactive",
		(sc_is_active(pd) &&
		 ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) ? " with SCBK-D" : ""
	);

	pd->state = next;
}

static void notify_command_status(struct osdp_pd *pd, int status)
{
	int app_cmd;
	struct osdp_event evt;
	struct osdp *ctx = pd_to_osdp(pd);

	if (!ctx->event_callback ||
	    !ISSET_FLAG(pd, OSDP_FLAG_ENABLE_NOTIFICATION)) {
		return;
	}

	switch (pd->cmd_id) {
	case CMD_OUT:    app_cmd = OSDP_CMD_OUTPUT; break;
	case CMD_LED:    app_cmd = OSDP_CMD_LED;    break;
	case CMD_BUZ:    app_cmd = OSDP_CMD_BUZZER; break;
	case CMD_TEXT:   app_cmd = OSDP_CMD_TEXT;   break;
	case CMD_COMSET: app_cmd = OSDP_CMD_COMSET; break;
	case CMD_ISTAT:  app_cmd = OSDP_CMD_STATUS; break;
	case CMD_OSTAT:  app_cmd = OSDP_CMD_STATUS; break;
	case CMD_LSTAT:  app_cmd = OSDP_CMD_STATUS; break;
	case CMD_RSTAT:  app_cmd = OSDP_CMD_STATUS; break;
	case CMD_KEYSET: app_cmd = OSDP_CMD_KEYSET; break;
	case CMD_MFG:
		if (pd->reply_id == REPLY_ACK) {
			app_cmd = OSDP_CMD_MFG;
			break;
		}
	default:
		return;
	}

	evt.type = OSDP_EVENT_NOTIFICATION;
	evt.notif.type = OSDP_EVENT_NOTIFICATION_COMMAND;
	evt.notif.arg0 = app_cmd;
	evt.notif.arg1 = status;

	ctx->event_callback(ctx->event_callback_arg, pd->idx, &evt);
}

static int state_update(struct osdp_pd *pd)
{
	int err;
	bool status;
	enum osdp_cp_state_e next, cur = pd->state;

	if (cp_phy_running(pd)) {
		err = cp_phy_state_update(pd);
		if (err == OSDP_CP_ERR_INPROG || err == OSDP_CP_ERR_CAN_YIELD) {
			return err;
		}
	}

	err = OSDP_CP_ERR_NONE;
	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_IDLE:
		pd->cmd_id = state_get_cmd(pd);
		if (pd->cmd_id > 0 && cp_phy_kick(pd)) {
			return OSDP_CP_ERR_CAN_YIELD;
		}
		break;
	case OSDP_CP_PHY_STATE_ERR:
		err = OSDP_CP_ERR_GENERIC;
		__fallthrough;
	case OSDP_CP_PHY_STATE_DONE:
		status = state_check_reply(pd);
		notify_command_status(pd, status);
		if (!status) {
			err = OSDP_CP_ERR_GENERIC;
		}
		osdp_phy_state_reset(pd, false);
		break;
	default:
		BUG();
	}

	next = get_next_state(pd, err);

	if (pd->state == OSDP_CP_STATE_ONLINE || next == OSDP_CP_STATE_ONLINE) {
		if (check_request(pd, CP_REQ_RESTART_SC)) {
			osdp_phy_state_reset(pd, true);
			next = OSDP_CP_STATE_SC_CHLNG;
		}
		if (check_request(pd, CP_REQ_OFFLINE)) {
			LOG_INF("Going offline due to request");
			next = OSDP_CP_STATE_OFFLINE;
		}
		if (check_request(pd, CP_REQ_EVENT_SEND)) {
			do_event_callback(pd);
		}
	}

	if (check_request(pd, CP_REQ_DISABLE)) {
		next = OSDP_CP_STATE_DISABLED;
	}

	if (check_request(pd, CP_REQ_ENABLE)) {
		next = OSDP_CP_STATE_INIT;
	}

	if (cur != next) {
		cp_state_change(pd, next);
	}
	return OSDP_CP_ERR_CAN_YIELD;
}

static int cp_refresh(struct osdp_pd *pd)
{
	int rc;
	struct osdp *ctx = pd_to_osdp(pd);

	if (ISSET_FLAG(pd, PD_FLAG_CHN_SHARED) &&
	    cp_channel_acquire(pd, NULL)) {
		/* Channel shared and failed to acquire lock */
		return 0;
	}

	rc = state_update(pd);

	if (ISSET_FLAG(pd, PD_FLAG_CHN_SHARED)) {
		if (rc == OSDP_CP_ERR_CAN_YIELD) {
			cp_channel_release(pd);
		} else if (ctx->num_channels == 1) {
			/**
			 * If there is only one channel, there is no point in
			 * trying to cp_channel_acquire() on the rest of the
			 * PDs when we know that can never succeed.
			 */
			return -1;
		}
	}
	return 0;
}

static int cp_submit_command(struct osdp_pd *pd, const struct osdp_cmd *cmd)
{
	struct osdp_cmd *p;
	const uint32_t all_flags = (
		OSDP_CMD_FLAG_BROADCAST
	);

	if (pd->state == OSDP_CP_STATE_DISABLED) {
		LOG_ERR("PD is disabled");
		return -1;
	}

	if (pd->state != OSDP_CP_STATE_ONLINE) {
		LOG_ERR("PD is not online");
		return -1;
	}

	if (cmd->flags & ~all_flags) {
		LOG_ERR("Invalid command flag");
		return -1;
	}

	if (cmd->flags & OSDP_CMD_FLAG_BROADCAST) {
		if (NUM_PD(pd->osdp_ctx) != 1) {
			LOG_ERR("Command broadcast is allowed only in single"
				" PD environments");
			return -1;
		}
		if (is_enforce_secure(pd)) {
			LOG_ERR("Cannot send command in broadcast mode"
				" due to ENFORCE_SECURE");
			return -1;
		}
	}

	if (cmd->id == OSDP_CMD_FILE_TX) {
		return osdp_file_tx_command(pd, cmd->file_tx.id,
					    cmd->file_tx.flags);
	} else if (cmd->id == OSDP_CMD_KEYSET &&
		   (cmd->keyset.type != 1 || !sc_is_active(pd))) {
		LOG_ERR("Invalid keyset request");
		return -1;
	}

	p = cp_cmd_alloc(pd);
	if (p == NULL) {
		LOG_ERR("Failed to allocate command");
		return -1;
	}
	memcpy(p, cmd, sizeof(struct osdp_cmd));
	cp_cmd_enqueue(pd, p);
	return 0;
}

static int cp_detect_connection_topology(struct osdp *ctx)
{
	int i, j, num_channels;
	int *channel_lock = NULL;
	struct osdp_pd *pd;
	struct disjoint_set set;
	int channels[OSDP_PD_MAX] = { 0 };

	if (disjoint_set_make(&set, NUM_PD(ctx)))
		return -1;

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		for (j = 0; j < i; j++) {
			if (channels[j] == pd->channel.id) {
				SET_FLAG(osdp_to_pd(ctx, j), PD_FLAG_CHN_SHARED);
				SET_FLAG(pd, PD_FLAG_CHN_SHARED);
				disjoint_set_union(&set, i, j);
			}
		}
		channels[i] = pd->channel.id;
	}

	num_channels = disjoint_set_num_roots(&set);
	if (num_channels != NUM_PD(ctx)) {
		channel_lock = calloc(1, sizeof(int) * NUM_PD(ctx));
		if (channel_lock == NULL) {
			LOG_PRINT("Failed to allocate osdp channel locks");
			return -1;
		}
	}

	safe_free(ctx->channel_lock);
	ctx->num_channels = num_channels;
	ctx->channel_lock = channel_lock;
	return 0;
}

static int cp_add_pd(struct osdp *ctx, int num_pd, const osdp_pd_info_t *info_list)
{
	int i, old_num_pd;
	struct osdp_pd *old_pd_array, *new_pd_array, *pd;
	const osdp_pd_info_t *info;
	char name[24] = { 0 };

	assert(num_pd);
	assert(info_list);
	old_num_pd = ctx->_num_pd;
	old_pd_array = ctx->pd;

	new_pd_array = calloc(old_num_pd + num_pd, sizeof(struct osdp_pd));
	if (new_pd_array == NULL) {
		LOG_PRINT("Failed to allocate new osdp_pd[] context");
		return -1;
	}

	ctx->pd = new_pd_array;
	ctx->_num_pd = old_num_pd + num_pd;
	memcpy(new_pd_array, old_pd_array, sizeof(struct osdp_pd) * old_num_pd);

	for (i = 0; i < num_pd; i++) {
		info = info_list + i;
		pd = osdp_to_pd(ctx, i + old_num_pd);
		pd->idx = i;
		pd->osdp_ctx = ctx;
		if (info->name) {
			strncpy(pd->name, info->name, OSDP_PD_NAME_MAXLEN - 1);
		} else {
			snprintf(pd->name, OSDP_PD_NAME_MAXLEN, "PD-%d", info->address);
		}
		pd->baud_rate = info->baud_rate;
		pd->address = info->address;
		pd->flags = info->flags;
		pd->seq_number = -1;
		SET_FLAG(pd, PD_FLAG_SC_DISABLED);
		/* Default to CRC-16 until we know PD capabilities */
		SET_FLAG(pd, PD_FLAG_CP_USE_CRC);
		memcpy(&pd->channel, &info->channel, sizeof(struct osdp_channel));
		if (info->scbk != NULL) {
			memcpy(pd->sc.scbk, info->scbk, 16);
			SET_FLAG(pd, PD_FLAG_HAS_SCBK);
			CLEAR_FLAG(pd, PD_FLAG_SC_DISABLED);
		} else if (is_enforce_secure(pd)) {
			LOG_PRINT("SCBK must be passed for each PD when"
				  " ENFORCE_SECURE is requested.");
			goto error;
		}
		if (cp_cmd_queue_init(pd)) {
			goto error;
		}
		if (IS_ENABLED(OPT_OSDP_SKIP_MARK_BYTE)) {
			SET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK);
		}

		logger_get_default(&pd->logger);
		snprintf(name, sizeof(name), "OSDP: CP: PD-%d", pd->address);
		logger_set_name(&pd->logger, name);

		if (is_capture_enabled(pd)) {
			osdp_packet_capture_init(pd);
		}
	}

	if (cp_detect_connection_topology(ctx)) {
		LOG_PRINT("Failed to detect connection topology");
		goto error;
	}

	SET_CURRENT_PD(ctx, 0);
	if (old_num_pd) {
		free(old_pd_array);
	}
	return 0;

error:
	ctx->pd = old_pd_array;
	ctx->_num_pd = old_num_pd;
	free(new_pd_array);
	return -1;
}

/* --- Exported Methods --- */

osdp_t *osdp_cp_setup(int num_pd, const osdp_pd_info_t *info)
{
	struct osdp *ctx;

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_PRINT("Failed to allocate osdp context");
		return NULL;
	}

	input_check_init(ctx);

	if (num_pd && cp_add_pd(ctx, num_pd, info)) {
		goto error;
	}

	LOG_PRINT("CP Setup complete; LibOSDP-%s %s NumPDs:%d Channels:%d",
		  osdp_get_version(), osdp_get_source_info(), num_pd,
		  ctx->num_channels);
	return ctx;
error:
	osdp_cp_teardown((osdp_t *)ctx);
	return NULL;
}

int osdp_cp_add_pd(osdp_t *ctx, int num_pd, const osdp_pd_info_t *info)
{
	input_check(ctx);
	assert(num_pd);
	assert(info);

	if(cp_add_pd(ctx, num_pd, info)) {
		LOG_PRINT("Failed to add PDs");
		return -1;
	}

	LOG_PRINT("Added %d PDs; TotalPDs:%d Channels:%d",
		  num_pd, ((struct osdp *)ctx)->_num_pd,
		  ((struct osdp *)ctx)->num_channels);
	return 0;
}

void osdp_cp_teardown(osdp_t *ctx)
{
	input_check(ctx);
	int i;
	struct osdp_pd *pd;

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		if (is_capture_enabled(pd)) {
			osdp_packet_capture_finish(pd);
		}
		safe_free(pd->file);
		if (pd->channel.close) {
			pd->channel.close(pd->channel.data);
		}
	}

	safe_free(osdp_to_pd(ctx, 0));
	safe_free(TO_OSDP(ctx)->channel_lock);
	safe_free(ctx);
}

void osdp_cp_refresh(osdp_t *ctx)
{
	input_check(ctx);
	int next_pd_idx, refresh_count = 0;
	struct osdp_pd *pd;

	while(refresh_count < NUM_PD(ctx)) {
		pd = GET_CURRENT_PD(ctx);

		if (cp_refresh(pd) < 0)
			break;

		next_pd_idx = pd->idx + 1;
		if (next_pd_idx >= NUM_PD(ctx)) {
			next_pd_idx = 0;
		}
		SET_CURRENT_PD(ctx, next_pd_idx);
		refresh_count++;
	}
}

void osdp_cp_set_event_callback(osdp_t *ctx, cp_event_callback_t cb, void *arg)
{
	input_check(ctx);

	TO_OSDP(ctx)->event_callback = cb;
	TO_OSDP(ctx)->event_callback_arg = arg;
}

int osdp_cp_send_command(osdp_t *ctx, int pd_idx, const struct osdp_cmd *cmd)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	return cp_submit_command(pd, cmd);
}

int osdp_cp_submit_command(osdp_t *ctx, int pd_idx, const struct osdp_cmd *cmd)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	return cp_submit_command(pd, cmd);
}

int osdp_cp_flush_commands(osdp_t *ctx, int pd_idx)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);
	struct osdp_cmd *cmd;
	int count = 0;

	while (cp_cmd_dequeue(pd, &cmd) == 0) {
		cp_cmd_free(pd, cmd);
		count++;
	}
	return count;
}

int osdp_cp_get_pd_id(const osdp_t *ctx, int pd_idx, struct osdp_pd_id *id)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	memcpy(id, &pd->id, sizeof(struct osdp_pd_id));
	return 0;
}

int osdp_cp_get_capability(const osdp_t *ctx, int pd_idx, struct osdp_pd_cap *cap)
{
	input_check(ctx, pd_idx);
	int fc;
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	fc = cap->function_code;
	if (fc <= OSDP_PD_CAP_UNUSED || fc >= OSDP_PD_CAP_SENTINEL) {
		return -1;
	}

	cap->compliance_level = pd->cap[fc].compliance_level;
	cap->num_items = pd->cap[fc].num_items;
	return 0;
}

int osdp_cp_modify_flag(osdp_t *ctx, int pd_idx, uint32_t flags, bool do_set)
{
	input_check(ctx, pd_idx);
	const uint32_t all_flags = (
		OSDP_FLAG_ENFORCE_SECURE |
		OSDP_FLAG_INSTALL_MODE |
		OSDP_FLAG_IGN_UNSOLICITED
	);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	if (flags & ~all_flags) {
		return -1;
	}

	do_set ? SET_FLAG(pd, flags) : CLEAR_FLAG(pd, flags);
	return 0;
}

int osdp_cp_disable_pd(osdp_t *ctx, int pd_idx)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	if (pd->state == OSDP_CP_STATE_DISABLED) {
		LOG_DBG("PD is already disabled");
		return -1;
	}

	if (test_request(pd, CP_REQ_DISABLE)) {
		LOG_DBG("PD disable request already pending");
		return -1;
	}

	make_request(pd, CP_REQ_DISABLE);
	return 0;
}

int osdp_cp_enable_pd(osdp_t *ctx, int pd_idx)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	if (pd->state != OSDP_CP_STATE_DISABLED) {
		LOG_DBG("PD is already enabled");
		return -1;
	}

	if (test_request(pd, CP_REQ_ENABLE)) {
		LOG_DBG("PD enable request already pending");
		return -1;
	}

	make_request(pd, CP_REQ_ENABLE);
	return 0;
}

bool osdp_cp_is_pd_enabled(const osdp_t *ctx, int pd_idx)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	return pd->state != OSDP_CP_STATE_DISABLED;
}

#ifdef UNIT_TESTING

/**
 * Force export some private methods for testing.
 */
void (*test_cp_cmd_enqueue)(struct osdp_pd *,
                            struct osdp_cmd *) = cp_cmd_enqueue;
struct osdp_cmd *(*test_cp_cmd_alloc)(struct osdp_pd *) = cp_cmd_alloc;
int (*test_cp_phy_state_update)(struct osdp_pd *) = cp_phy_state_update;
int (*test_state_update)(struct osdp_pd *) = state_update;
int (*test_cp_build_and_send_packet)(struct osdp_pd *pd) = cp_build_and_send_packet;
const int CP_ERR_CAN_YIELD = OSDP_CP_ERR_CAN_YIELD;
const int CP_ERR_INPROG = OSDP_CP_ERR_INPROG;

#endif /* UNIT_TESTING */
