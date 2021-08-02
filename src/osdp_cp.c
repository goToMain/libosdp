/*
 * Copyright (c) 2019-2021-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "osdp_common.h"
#include "osdp_file.h"

#define LOG_TAG "CP: "

#define CMD_POLL_LEN   1
#define CMD_LSTAT_LEN  1
#define CMD_ISTAT_LEN  1
#define CMD_OSTAT_LEN  1
#define CMD_RSTAT_LEN  1
#define CMD_ID_LEN     2
#define CMD_CAP_LEN    2
#define CMD_DIAG_LEN   2
#define CMD_OUT_LEN    5
#define CMD_LED_LEN    15
#define CMD_BUZ_LEN    6
#define CMD_TEXT_LEN   7 /* variable length command */
#define CMD_COMSET_LEN 6
#define CMD_MFG_LEN    4 /* variable length command */
#define CMD_KEYSET_LEN 19
#define CMD_CHLNG_LEN  9
#define CMD_SCRYPT_LEN 17

#define REPLY_ACK_DATA_LEN     0
#define REPLY_PDID_DATA_LEN    12
#define REPLY_PDCAP_ENTITY_LEN 3
#define REPLY_LSTATR_DATA_LEN  2
#define REPLY_RSTATR_DATA_LEN  1
#define REPLY_COM_DATA_LEN     5
#define REPLY_NAK_DATA_LEN     1
#define REPLY_MFGREP_LEN       4 /* variable length command */
#define REPLY_CCRYPT_DATA_LEN  32
#define REPLY_RMAC_I_DATA_LEN  16
#define REPLY_KEYPPAD_DATA_LEN 2 /* variable length command */
#define REPLY_RAW_DATA_LEN     4 /* variable length command */
#define REPLY_FMT_DATA_LEN     3 /* variable length command */
#define REPLY_BUSY_DATA_LEN    0

enum osdp_cp_error_e {
	OSDP_CP_ERR_NONE = 0,
	OSDP_CP_ERR_GENERIC = -1,
	OSDP_CP_ERR_NO_DATA = -2,
	OSDP_CP_ERR_RETRY_CMD = -3,
	OSDP_CP_ERR_CAN_YIELD = -4,
	OSDP_CP_ERR_INPROG = -5,
};

struct cp_cmd_node {
	queue_node_t node;
	struct osdp_cmd object;
};

static int cp_cmd_queue_init(struct osdp_pd *pd)
{
	if (slab_init(&pd->cmd.slab, sizeof(struct cp_cmd_node),
		      pd->cmd.slab_blob, sizeof(pd->cmd.slab_blob)) < 0) {
		LOG_ERR("Failed to initialize command slab");
		return -1;
	}
	queue_init(&pd->cmd.queue);
	return 0;
}

static struct osdp_cmd *cp_cmd_alloc(struct osdp_pd *pd)
{
	struct cp_cmd_node *cmd = NULL;

	if (slab_alloc(&pd->cmd.slab, (void **)&cmd)) {
		LOG_ERR("Command slab allocation failed");
		return NULL;
	}
	memset(&cmd->object, 0, sizeof(cmd->object));
	return &cmd->object;
}

static void cp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	ARG_UNUSED(pd);
	struct cp_cmd_node *n;

	n = CONTAINER_OF(cmd, struct cp_cmd_node, object);
	if (slab_free(n)) {
		LOG_ERR("Command slab free failed");
	}
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
		[OSDP_PD_CAP_READER_AUDIBLE_OUTPUT] = "AudiableControl",
		[OSDP_PD_CAP_READER_TEXT_OUTPUT] = "TextOutput",
		[OSDP_PD_CAP_TIME_KEEPING] = "TimeKeeping",
		[OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT] = "CheckCharacter",
		[OSDP_PD_CAP_COMMUNICATION_SECURITY] = "CommunicationSecurity",
		[OSDP_PD_CAP_RECEIVE_BUFFERSIZE] = "ReceiveBufferSize",
		[OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE] = "CombinedMessageSize",
		[OSDP_PD_CAP_SMART_CARD_SUPPORT] = "SmartCard",
		[OSDP_PD_CAP_READERS] = "Reader",
		[OSDP_PD_CAP_BIOMETRICS] = "Biometric",
	};
	return cap_name[cap];
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

#define ASSERT_BUF_LEN(need)                                                   \
	if (max_len < need) {                                                  \
		LOG_ERR("OOM at build CMD(%02x) - have:%d, need:%d",           \
			pd->cmd_id, max_len, need);                            \
		return OSDP_CP_ERR_GENERIC;                                    \
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
	case CMD_LSTAT:
	case CMD_ISTAT:
	case CMD_OSTAT:
	case CMD_RSTAT:
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_ID:
		ASSERT_BUF_LEN(CMD_ID_LEN);
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_CAP:
		ASSERT_BUF_LEN(CMD_CAP_LEN);
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_DIAG:
		ASSERT_BUF_LEN(CMD_DIAG_LEN);
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_OUT:
		ASSERT_BUF_LEN(CMD_OUT_LEN);
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.timer_count);
		buf[len++] = BYTE_1(cmd->output.timer_count);
		ret = 0;
		break;
	case CMD_LED:
		ASSERT_BUF_LEN(CMD_LED_LEN);
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
		ASSERT_BUF_LEN(CMD_BUZ_LEN);
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
		ASSERT_BUF_LEN(CMD_TEXT_LEN + cmd->text.length);
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
		ASSERT_BUF_LEN(CMD_COMSET_LEN);
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
		ASSERT_BUF_LEN(CMD_MFG_LEN + cmd->mfg.length);
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
	case CMD_ACURXSIZE:
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(OSDP_PACKET_BUF_SIZE);
		buf[len++] = BYTE_1(OSDP_PACKET_BUF_SIZE);
		ret = 0;
		break;
	case CMD_KEEPACTIVE:
		buf[len++] = pd->cmd_id;
		buf[len++] = BYTE_0(0); // keepalive in ms time LSB
		buf[len++] = BYTE_1(0); // keepalive in ms time MSB
		ret = 0;
		break;
	case CMD_ABORT:
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_FILETRANSFER:
		buf[len++] = pd->cmd_id;
		ret = osdp_file_cmd_tx_build(pd, buf + len, max_len);
		if (ret <= 0) {
			break;
		}
		len += ret;
		break;
	case CMD_KEYSET:
		if (!sc_is_active(pd)) {
			LOG_ERR("Can not perform a KEYSET without SC!");
			return -1;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		ASSERT_BUF_LEN(CMD_KEYSET_LEN);
		if (cmd->keyset.length != 16) {
			LOG_ERR("Invalid key length");
			return -1;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 1; /* key type is always SCBK here */
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
		ret = 0;
		break;
	case CMD_CHLNG:
		ASSERT_BUF_LEN(CMD_CHLNG_LEN);
		if (smb == NULL) {
			break;
		}
		smb[0] = 3; /* length */
		smb[1] = SCS_11; /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		for (i = 0; i < 8; i++)
			buf[len++] = pd->sc.cp_random[i];
		ret = 0;
		break;
	case CMD_SCRYPT:
		ASSERT_BUF_LEN(CMD_SCRYPT_LEN);
		if (smb == NULL) {
			break;
		}
		osdp_compute_cp_cryptogram(pd);
		smb[0] = 3; /* length */
		smb[1] = SCS_13; /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		for (i = 0; i < 16; i++)
			buf[len++] = pd->sc.cp_cryptogram[i];
		ret = 0;
		break;
	default:
		LOG_ERR("Unknown/Unsupported CMD(%02x)", pd->cmd_id);
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

	if (ret < 0) {
		LOG_ERR("Unable to build CMD(%02x)", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (IS_ENABLED(CONFIG_OSDP_DATA_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump(buf + 1, len - 1, "OSDP: CMD: %s(%02x)",
				  osdp_cmd_name(pd->cmd_id), pd->cmd_id);
		}
	}

	return len;
}

static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp *ctx = pd_to_osdp(pd);
	int i, ret = OSDP_CP_ERR_GENERIC, pos = 0, t1, t2;
	struct osdp_event event;

	pd->reply_id = buf[pos++];
	len--; /* consume reply id from the head */

	if (IS_ENABLED(CONFIG_OSDP_DATA_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump(buf, len, "OSDP: REPLY: %s(%02x)",
				  osdp_reply_name(pd->reply_id), pd->reply_id);
		}
	}

#define ASSERT_LENGTH(got, exp)                                                \
	if (got != exp) {                                                      \
		LOG_ERR("REPLY(%02x) length error! Got:%d, Exp:%d",            \
			pd->reply_id, got, exp);                               \
		return OSDP_CP_ERR_GENERIC;                                    \
	}

	switch (pd->reply_id) {
	case REPLY_ACK:
		ASSERT_LENGTH(len, REPLY_ACK_DATA_LEN);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_NAK:
		ASSERT_LENGTH(len, REPLY_NAK_DATA_LEN);
		LOG_WRN("PD replied with NAK(%d) for CMD(%02x)", buf[pos],
			pd->cmd_id);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_PDID:
		ASSERT_LENGTH(len, REPLY_PDID_DATA_LEN);
		pd->id.vendor_code = buf[pos++];
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
			LOG_ERR("PDCAP response length is not a multiple of 3",
				buf[pos], pd->cmd_id);
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
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_LSTATR:
		ASSERT_LENGTH(len, REPLY_LSTATR_DATA_LEN);
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
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RSTATR:
		ASSERT_LENGTH(len, REPLY_RSTATR_DATA_LEN);
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_R_TAMPER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_R_TAMPER);
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_COM:
		ASSERT_LENGTH(len, REPLY_COM_DATA_LEN);
		t1 = buf[pos++];
		temp32 = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_WRN("COMSET responded with ID:%d Baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_KEYPPAD:
		if (len < REPLY_KEYPPAD_DATA_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_KEYPRESS;
		event.keypress.reader_no = buf[pos++];
		event.keypress.length = buf[pos++]; /* key length */
		if ((len - REPLY_KEYPPAD_DATA_LEN) != event.keypress.length) {
			break;
		}
		for (i = 0; i < event.keypress.length; i++) {
			event.keypress.data[i] = buf[pos + i];
		}
		ctx->event_callback(ctx->event_callback_arg, pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN || !ctx->event_callback) {
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
		for (i = 0; i < t1; i++) {
			event.cardread.data[i] = buf[pos + i];
		}
		ctx->event_callback(ctx->event_callback_arg,
				    pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_FMT:
		if (len < REPLY_FMT_DATA_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.direction = buf[pos++];
		event.cardread.length = buf[pos++];
		event.cardread.format = OSDP_CARD_FMT_ASCII;
		if (event.cardread.length != (len - REPLY_FMT_DATA_LEN) ||
		    event.cardread.length > OSDP_EVENT_MAX_DATALEN) {
			break;
		}
		for (i = 0; i < event.cardread.length; i++) {
			event.cardread.data[i] = buf[pos + i];
		}
		ctx->event_callback(ctx->event_callback_arg,
				    pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		ASSERT_LENGTH(len, REPLY_BUSY_DATA_LEN);
		ret = OSDP_CP_ERR_RETRY_CMD;
		break;
	case REPLY_MFGREP:
		if (len < REPLY_MFGREP_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_MFGREP;
		event.mfgrep.vendor_code = buf[pos++];
		event.mfgrep.vendor_code |= buf[pos++] << 8;
		event.mfgrep.vendor_code |= buf[pos++] << 16;
		event.mfgrep.command = buf[pos++];
		event.mfgrep.length = len - REPLY_MFGREP_LEN;
		if (event.mfgrep.length > OSDP_EVENT_MAX_DATALEN) {
			break;
		}
		for (i = 0; i < event.mfgrep.length; i++) {
			event.mfgrep.data[i] = buf[pos + i];
		}
		ctx->event_callback(ctx->event_callback_arg,
				    pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_FTSTAT:
		ret = osdp_file_cmd_stat_decode(pd, buf + pos, len);
		break;
	case REPLY_CCRYPT:
		ASSERT_LENGTH(len, REPLY_CCRYPT_DATA_LEN);
		for (i = 0; i < 8; i++) {
			pd->sc.pd_client_uid[i] = buf[pos++];
		}
		for (i = 0; i < 8; i++) {
			pd->sc.pd_random[i] = buf[pos++];
		}
		for (i = 0; i < 16; i++) {
			pd->sc.pd_cryptogram[i] = buf[pos++];
		}
		osdp_compute_session_keys(pd);
		if (osdp_verify_pd_cryptogram(pd) != 0) {
			LOG_ERR("Failed to verify PD cryptogram");
			return OSDP_CP_ERR_GENERIC;
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RMAC_I:
		ASSERT_LENGTH(len, REPLY_RMAC_I_DATA_LEN);
		for (i = 0; i < 16; i++) {
			pd->sc.r_mac[i] = buf[pos++];
		}
		sc_activate(pd);
		ret = OSDP_CP_ERR_NONE;
		break;
	default:
		LOG_ERR("Unexpected REPLY(%02x)", pd->reply_id);
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG("CMD: %s(%02x) REPLY: %s(%02x)",
			osdp_cmd_name(pd->cmd_id), pd->cmd_id,
			osdp_reply_name(pd->reply_id), pd->reply_id);
	}

	return ret;
}

static int cp_build_packet(struct osdp_pd *pd)
{
	int ret, len;

	/* init packet buf with header */
	len = osdp_phy_packet_init(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	/* fill command data */
	ret = cp_build_command(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (ret < 0) {
		return OSDP_CP_ERR_GENERIC;
	}
	len += ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, pd->rx_buf, len, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	pd->rx_buf_len = len;

	return OSDP_CP_ERR_NONE;
}

static int cp_send_command(struct osdp_pd *pd)
{
	int ret;

	/* flush rx to remove any invalid data. */
	if (pd->channel.flush) {
		pd->channel.flush(pd->channel.data);
	}

	ret = pd->channel.send(pd->channel.data, pd->rx_buf, pd->rx_buf_len);
	if (ret != pd->rx_buf_len) {
		LOG_ERR("Channel send for %d bytes failed! ret: %d",
			pd->rx_buf_len, ret);
		return OSDP_CP_ERR_GENERIC;
	}

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump(pd->rx_buf, pd->rx_buf_len,
				  "OSDP: CP->PD[%d]: Sent", pd->idx);
		}
	}

	return OSDP_CP_ERR_NONE;
}

static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int err, len, one_pkt_len, remaining;

	buf = pd->rx_buf + pd->rx_buf_len;
	remaining = sizeof(pd->rx_buf) - pd->rx_buf_len;

	len = pd->channel.recv(pd->channel.data, buf, remaining);
	if (len <= 0) { /* No data received */
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len += len;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump(pd->rx_buf, pd->rx_buf_len,
				  "OSDP: CP->PD[%d]: Received", pd->idx);
		}
	}

	err = osdp_phy_check_packet(pd, pd->rx_buf, pd->rx_buf_len, &len);
	if (err == OSDP_ERR_PKT_WAIT) {
		/* rx_buf_len < pkt->len; wait for more data */
		return OSDP_CP_ERR_NO_DATA;
	}
	one_pkt_len = len;

	/* Translate phy error codes to CP errors */
	switch (err) {
	case OSDP_ERR_PKT_NONE: err = OSDP_CP_ERR_NONE; break;
	case OSDP_ERR_PKT_BUSY: err = OSDP_CP_ERR_RETRY_CMD; break;
	default: return OSDP_CP_ERR_GENERIC;
	}

	if (err == OSDP_ERR_PKT_NONE) {
		/* Valid OSDP packet in buffer */
		len = osdp_phy_decode_packet(pd, pd->rx_buf, len, &buf);
		if (len <= 0) {
			return OSDP_CP_ERR_GENERIC;
		}
		err = cp_decode_response(pd, buf, len);
	}

	/* We are done with the packet (error or not). Remove processed bytes */
	remaining = pd->rx_buf_len - one_pkt_len;
	if (remaining) {
		memmove(pd->rx_buf, pd->rx_buf + one_pkt_len, remaining);
	}
	pd->rx_buf_len = remaining;

	return err;
}

static void cp_flush_command_queue(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	while (cp_cmd_dequeue(pd, &cmd) == 0) {
		cp_cmd_free(pd, cmd);
	}
}

static inline void cp_set_state(struct osdp_pd *pd, enum osdp_state_e state)
{
	pd->state = state;
	CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
}

static inline void cp_set_online(struct osdp_pd *pd)
{
	cp_set_state(pd, OSDP_CP_STATE_ONLINE);
	pd->wait_ms = 0;
	pd->tstamp = 0;
}

static inline void cp_set_offline(struct osdp_pd *pd)
{
	sc_deactivate(pd);
	pd->state = OSDP_CP_STATE_OFFLINE;
	pd->tstamp = osdp_millis_now();
	if (pd->wait_ms == 0) {
		pd->wait_ms = 1000; /* retry after 1 second initially */
	} else {
		pd->wait_ms <<= 1;
		if (pd->wait_ms > OSDP_ONLINE_RETRY_WAIT_MAX_MS) {
			pd->wait_ms = OSDP_ONLINE_RETRY_WAIT_MAX_MS;
		}
	}
}

static inline bool cp_sc_should_retry(struct osdp_pd *pd)
{
	return (sc_is_capable(pd) && !sc_is_active(pd) &&
	        osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_MS);
}

static int cp_phy_state_update(struct osdp_pd *pd)
{
	int64_t elapsed;
	int rc, ret = OSDP_CP_ERR_CAN_YIELD;
	struct osdp_cmd *cmd = NULL;
	struct osdp *ctx = pd_to_osdp(pd);

	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_WAIT:
		elapsed = osdp_millis_since(pd->phy_tstamp);
		if (elapsed < OSDP_CMD_RETRY_WAIT_MS) {
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_SEND_CMD;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_IDLE:
		if (cp_cmd_dequeue(pd, &cmd)) {
			ret = OSDP_CP_ERR_NONE; /* command queue is empty */
			break;
		}
		pd->cmd_id = cmd->id;
		memcpy(pd->ephemeral_data, cmd, sizeof(struct osdp_cmd));
		cp_cmd_free(pd, cmd);
		/* fall-thru */
	case OSDP_CP_PHY_STATE_SEND_CMD:
		if (cp_build_packet(pd)) {
			LOG_ERR("Failed to build packet for CMD(%d)",
				pd->cmd_id);
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		if (cp_send_command(pd)) {
			LOG_ERR("Failed to send CMD(%d)", pd->cmd_id);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		ret = OSDP_CP_ERR_INPROG;
		pd->phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		rc = cp_process_reply(pd);
		if (rc == OSDP_CP_ERR_NONE) {
			if (ctx->command_complete_callback) {
				ctx->command_complete_callback(pd->cmd_id);
			}
			if (sc_is_active(pd)) {
				pd->sc_tstamp = osdp_millis_now();
			}
			pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
			break;
		}
		if (rc == OSDP_CP_ERR_RETRY_CMD) {
			LOG_INF("PD busy; retry last command");
			pd->phy_tstamp = osdp_millis_now();
			pd->phy_state = OSDP_CP_PHY_STATE_WAIT;
			break;
		}
		if (rc == OSDP_CP_ERR_GENERIC ||
		    osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			if (rc != OSDP_CP_ERR_GENERIC) {
				LOG_ERR("Response timeout for CMD(%02x)",
					pd->cmd_id);
			}
			pd->rx_buf_len = 0;
			if (pd->channel.flush) {
				pd->channel.flush(pd->channel.data);
			}
			cp_flush_command_queue(pd);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		ret = OSDP_CP_ERR_INPROG;
		break;
	}

	return ret;
}

static int cp_cmd_dispatcher(struct osdp_pd *pd, int cmd)
{
	struct osdp_cmd *c;

	if (ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
		CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
		return OSDP_CP_ERR_NONE; /* nothing to be done here */
	}

	c = cp_cmd_alloc(pd);
	if (c == NULL) {
		return OSDP_CP_ERR_GENERIC;
	}

	c->id = cmd;

	if (c->id == CMD_KEYSET) {
		memcpy(&c->keyset, pd->ephemeral_data, sizeof(c->keyset));
	}

	cp_cmd_enqueue(pd, c);
	SET_FLAG(pd, PD_FLAG_AWAIT_RESP);
	return OSDP_CP_ERR_INPROG;
}

static int state_update(struct osdp_pd *pd)
{
	bool soft_fail;
	int phy_state;
	struct osdp *ctx = pd_to_osdp(pd);
	struct osdp_cmd_keyset *keyset;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == OSDP_CP_ERR_INPROG ||
	    phy_state == OSDP_CP_ERR_CAN_YIELD) {
		return phy_state;
	}

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->state == OSDP_CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->state != OSDP_CP_STATE_OFFLINE &&
	    phy_state == OSDP_CP_ERR_GENERIC && !soft_fail) {
		cp_set_offline(pd);
		return OSDP_CP_ERR_CAN_YIELD;
	}

	/* command queue is empty and last command was successful */

	switch (pd->state) {
	case OSDP_CP_STATE_ONLINE:
		if (cp_sc_should_retry(pd)) {
			LOG_INF("Retry SC after retry timeout");
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
		if (osdp_file_tx_pending(pd)) {
			/* Don't care if dispatch failed; osdp_file.c should
			 * handle failures */
			cp_cmd_dispatcher(pd, CMD_FILETRANSFER);
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
		if (osdp_millis_since(pd->tstamp) > pd->wait_ms) {
			cp_set_state(pd, OSDP_CP_STATE_INIT);
			osdp_phy_state_reset(pd);
		}
		break;
	case OSDP_CP_STATE_INIT:
		cp_set_state(pd, OSDP_CP_STATE_IDREQ);
		__fallthrough;
	case OSDP_CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDID) {
			LOG_ERR("Unexpected REPLY(%02x) for cmd " STR(CMD_CAP),
				pd->reply_id);
			cp_set_offline(pd);
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_CAPDET);
		__fallthrough;
	case OSDP_CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDCAP) {
			LOG_ERR("Unexpected REPLY(%02x) for cmd " STR(CMD_CAP),
				pd->reply_id);
			cp_set_offline(pd);
			break;
		}
		if (sc_is_capable(pd)) {
			CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
		if (is_enforce_secure(pd)) {
			LOG_INF("SC disabled or not capable. Set PD offline due "
				"to ENFORCE_SECURE");
			cp_set_offline(pd);
		} else {
			cp_set_online(pd);
		}
		break;
	case OSDP_CP_STATE_SC_INIT:
		osdp_sc_setup(pd);
		cp_set_state(pd, OSDP_CP_STATE_SC_CHLNG);
		__fallthrough;
	case OSDP_CP_STATE_SC_CHLNG:
		if (cp_cmd_dispatcher(pd, CMD_CHLNG) != 0) {
			break;
		}
		if (phy_state < 0) {
			if (is_enforce_secure(pd)) {
				LOG_INF("SC Failed. Set PD offline due to "
					"ENFORCE_SECURE");
				cp_set_offline(pd);
				break;
			}
			if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
				LOG_INF("SC Failed. Online without SC");
				pd->sc_tstamp = osdp_millis_now();
				osdp_phy_state_reset(pd);
				cp_set_online(pd);
				break;
			}
			SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			pd->phy_state = 0; /* soft reset phy state */
			LOG_WRN("SC Failed. Retry with SCBK-D");
			break;
		}
		if (pd->reply_id != REPLY_CCRYPT) {
			if (is_enforce_secure(pd)) {
				LOG_ERR("CHLNG failed. Set PD offline due to "
					"ENFORCE_SECURE");
				cp_set_offline(pd);
			} else {
				LOG_ERR("CHLNG failed. Online without SC");
				pd->sc_tstamp = osdp_millis_now();
				osdp_phy_state_reset(pd);
				cp_set_online(pd);
			}
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_SC_SCRYPT);
		__fallthrough;
	case OSDP_CP_STATE_SC_SCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_RMAC_I) {
			if (is_enforce_secure(pd)) {
				LOG_ERR("SCRYPT failed. Set PD offline due to "
					"ENFORCE_SECURE");
				cp_set_offline(pd);
			} else {
				LOG_ERR("SCRYPT failed. Online without SC");
				osdp_phy_state_reset(pd);
				pd->sc_tstamp = osdp_millis_now();
				cp_set_online(pd);
			}
			break;
		}
		if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_WRN("SC ACtive with SCBK-D. Set SCBK");
			cp_set_state(pd, OSDP_CP_STATE_SET_SCBK);
			break;
		}
		LOG_INF("SC Active");
		pd->sc_tstamp = osdp_millis_now();
		cp_set_online(pd);
		break;
	case OSDP_CP_STATE_SET_SCBK:
		if (!ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
			keyset = (struct osdp_cmd_keyset *)pd->ephemeral_data;
			if (ISSET_FLAG(pd, PD_FLAG_HAS_SCBK)) {
				memcpy(keyset->data, pd->sc.scbk, 16);
				keyset->type = 1;
			} else {
				keyset->type = 0;
				memcpy(keyset->data, ctx->sc_master_key, 16);
			}
			keyset->length = 16;
		}
		if (cp_cmd_dispatcher(pd, CMD_KEYSET) != 0) {
			break;
		}
		if (pd->reply_id == REPLY_NAK) {
			if (is_enforce_secure(pd)) {
				LOG_ERR("Failed to set SCBK; "
					"Set PD offline due to ENFORCE_SECURE");
				cp_set_offline(pd);
			} else {
				LOG_WRN("Failed to set SCBK; "
					"Continue with SCBK-D");
				cp_set_online(pd);
			}
			break;
		}
		osdp_keyset_complete(pd);
		pd->seq_number = -1;
		break;
	default:
		break;
	}

	return OSDP_CP_ERR_CAN_YIELD;
}

static int osdp_cp_send_command_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p)
{
	int i, res = 0;
	struct osdp_cmd *cmd[OSDP_PD_MAX] = { 0 };
	struct osdp_pd *pd;

	if (osdp_get_sc_status_mask(ctx) != PD_MASK(ctx)) {
		LOG_WRN("master_key based key set can be performed only when "
			"all PDs are ONLINE, SC_ACTIVE");
		return -1;
	}

	if (NUM_PD(ctx) > OSDP_PD_MAX) {
		return -1;
	}

	LOG_INF("master_key based key set is a global command; "
		"all connected PDs will be affected.");

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		cmd[i] = cp_cmd_alloc(pd);
		if (cmd[i] == NULL) {
			res = -1;
			break;
		}
		cmd[i]->id = CMD_KEYSET;
		memcpy(&cmd[i]->keyset, p, sizeof(struct osdp_cmd_keyset));
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		if (res < 0 && cmd[i]) {
			cp_cmd_free(pd, cmd[i]);
		} else {
			cp_cmd_enqueue(pd, cmd[i]);
		}
	}

	return res;
}

void osdp_keyset_complete(struct osdp_pd *pd)
{
	struct osdp_cmd *c = (struct osdp_cmd *)pd->ephemeral_data;

	sc_deactivate(pd);
	CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
	memcpy(pd->sc.scbk, c->keyset.data, 16);
	cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
	LOG_INF("SCBK set; restarting SC to verify new SCBK");
}

/* --- Exported Methods --- */

OSDP_EXPORT
osdp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key)
{
	int i, owner, scbk_count = 0;
	struct osdp_pd *pd;
	struct osdp *ctx;

	assert(info);
	assert(num_pd > 0);

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_ERR("Failed to allocate osdp context");
		return NULL;
	}

	input_check_init(ctx);

	ctx->channel_lock = calloc(1, sizeof(int) * num_pd);
	if (ctx->channel_lock == NULL) {
		LOG_ERR("Failed to allocate osdp channel locks");
		goto error;
	}

	ctx->pd = calloc(1, sizeof(struct osdp_pd) * num_pd);
	if (ctx->pd == NULL) {
		LOG_ERR("Failed to allocate osdp_pd[] context");
		goto error;
	}
	ctx->_num_pd = num_pd;

	for (i = 0; i < num_pd; i++) {
		osdp_pd_info_t *p = info + i;
		pd = osdp_to_pd(ctx, i);
		pd->idx = i;
		pd->osdp_ctx = ctx;
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->flags;
		pd->seq_number = -1;
		SET_FLAG(pd, PD_FLAG_SC_DISABLED);
		if (p->scbk != NULL) {
			scbk_count += 1;
			memcpy(pd->sc.scbk, p->scbk, 16);
			SET_FLAG(pd, PD_FLAG_HAS_SCBK);
			CLEAR_FLAG(pd, PD_FLAG_SC_DISABLED);
		} else if (is_enforce_secure(pd)) {
			LOG_ERR("SCBK must be passed for each PD when"
				" ENFORCE_SECURE is requested.");
			goto error;
		}
		if (cp_cmd_queue_init(pd)) {
			goto error;
		}
		memcpy(&pd->channel, &p->channel, sizeof(struct osdp_channel));
		if (cp_channel_acquire(pd, &owner) == -1) {
			SET_FLAG(osdp_to_pd(ctx, owner), PD_FLAG_CHN_SHARED);
			SET_FLAG(pd, PD_FLAG_CHN_SHARED);
		}
		if (IS_ENABLED(CONFIG_OSDP_SKIP_MARK_BYTE)) {
			SET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK);
		}
	}

	if (scbk_count != num_pd) {
		if (master_key != NULL) {
			LOG_WRN("Masterkey based key derivation is discouraged!"
				" Consider passing individual SCBKs");
			memcpy(ctx->sc_master_key, master_key, 16);
			for (i = 0; i < num_pd; i++) {
				CLEAR_FLAG(pd, PD_FLAG_SC_DISABLED);
			}
		}
	}

	memset(ctx->channel_lock, 0, sizeof(int) * num_pd);
	SET_CURRENT_PD(ctx, 0);

	LOG_INF("CP setup (with %d connected PDs) complete - %s %s",
		num_pd, osdp_get_version(), osdp_get_source_info());

	return (osdp_t *)ctx;
error:
	osdp_cp_teardown((osdp_t *)ctx);
	return NULL;
}

OSDP_EXPORT
void osdp_cp_teardown(osdp_t *ctx)
{
	input_check(ctx);

	safe_free(osdp_to_pd(ctx, 0));
	safe_free(TO_OSDP(ctx)->channel_lock);
	safe_free(ctx);
}

OSDP_EXPORT
void osdp_cp_refresh(osdp_t *ctx)
{
	input_check(ctx);
	int i, rc;
	struct osdp_pd *pd;

	for (i = 0; i < NUM_PD(ctx); i++) {
		SET_CURRENT_PD(ctx, i);
		pd = osdp_to_pd(ctx, i);
		osdp_log_ctx_set(pd->address);

		if (ISSET_FLAG(pd, PD_FLAG_CHN_SHARED) &&
		    cp_channel_acquire(pd, NULL)) {
			/* failed to lock shared channel */
			continue;
		}

		rc = state_update(pd);

		if (ISSET_FLAG(pd, PD_FLAG_CHN_SHARED) &&
		    rc == OSDP_CP_ERR_CAN_YIELD) {
			cp_channel_release(pd);
		}
	}
}

OSDP_EXPORT
void osdp_cp_set_event_callback(osdp_t *ctx, cp_event_callback_t cb, void *arg)
{
	input_check(ctx);

	TO_OSDP(ctx)->event_callback = cb;
	TO_OSDP(ctx)->event_callback_arg = arg;
}

OSDP_EXPORT
int osdp_cp_send_command(osdp_t *ctx, int pd_idx, struct osdp_cmd *p)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);
	struct osdp_cmd *cmd;
	int cmd_id;

	if (pd->state != OSDP_CP_STATE_ONLINE) {
		LOG_PRINT("PD not online");
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
	case OSDP_CMD_FILE_TX:
		return osdp_file_tx_initiate(pd, p->file_tx.id,
					     p->file_tx.flags);
	case OSDP_CMD_KEYSET:
		if (p->keyset.type == 1) {
			if (!sc_is_active(pd))
				return -1;
			cmd_id = CMD_KEYSET;
			break;
		}
		if (ISSET_FLAG(pd, OSDP_FLAG_ENFORCE_SECURE)) {
			LOG_PRINT("master_key based key set blocked "
				  "due to ENFORCE_SECURE;");
			return -1;
		}
		return osdp_cp_send_command_keyset(ctx, &p->keyset);
	default:
		LOG_PRINT("Invalid CMD_ID:%d", p->id);
		return -1;
	}

	cmd = cp_cmd_alloc(pd);
	if (cmd == NULL) {
		return -1;
	}

	memcpy(cmd, p, sizeof(struct osdp_cmd));
	cmd->id = cmd_id; /* translate to internal */
	cp_cmd_enqueue(pd, cmd);
	return 0;
}

OSDP_EXPORT
int osdp_cp_get_pd_id(osdp_t *ctx, int pd_idx, struct osdp_pd_id *id)
{
	input_check(ctx, pd_idx);
	struct osdp_pd *pd = osdp_to_pd(ctx, pd_idx);

	memcpy(id, &pd->id, sizeof(struct osdp_pd_id));
	return 0;
}

OSDP_EXPORT
int osdp_cp_get_capability(osdp_t *ctx, int pd_idx, struct osdp_pd_cap *cap)
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