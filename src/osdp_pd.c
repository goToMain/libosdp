/*
 * Copyright (c) 2019-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "osdp_common.h"
#include "osdp_file.h"
#include "osdp_diag.h"

#ifndef OPT_OSDP_STATIC_PD
#include <stdlib.h>
#endif

#define CMD_POLL_DATA_LEN              0
#define CMD_LSTAT_DATA_LEN             0
#define CMD_ISTAT_DATA_LEN             0
#define CMD_OSTAT_DATA_LEN             0
#define CMD_RSTAT_DATA_LEN             0
#define CMD_ID_DATA_LEN                1
#define CMD_CAP_DATA_LEN               1
#define CMD_OUT_DATA_LEN               4
#define CMD_LED_DATA_LEN               14
#define CMD_BUZ_DATA_LEN               5
#define CMD_TEXT_DATA_LEN              6   /* variable length command */
#define CMD_COMSET_DATA_LEN            5
#define CMD_KEYSET_DATA_LEN            18
#define CMD_CHLNG_DATA_LEN             8
#define CMD_SCRYPT_DATA_LEN            16
#define CMD_ABORT_DATA_LEN             0
#define CMD_ACURXSIZE_DATA_LEN         2
#define CMD_KEEPACTIVE_DATA_LEN        2
#define CMD_MFG_DATA_LEN               3 /* variable length command */

#define REPLY_ACK_LEN                  1
#define REPLY_PDID_LEN                 13
#define REPLY_PDCAP_LEN                1   /* variable length command */
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_LEN               3
#define REPLY_RSTATR_LEN               2
#define REPLY_COM_LEN                  6
#define REPLY_NAK_LEN                  2
#define REPLY_CCRYPT_LEN               33
#define REPLY_RMAC_I_LEN               17
#define REPLY_KEYPAD_LEN               2
#define REPLY_RAW_LEN                  4
#define REPLY_MFGREP_LEN               4 /* variable length command */

enum osdp_pd_error_e {
	OSDP_PD_ERR_NONE = 0,
	OSDP_PD_ERR_WAIT = -1,
	OSDP_PD_ERR_GENERIC = -2,
	OSDP_PD_ERR_REPLY = -3,
	OSDP_PD_ERR_IGNORE = -4,
	OSDP_PD_ERR_NO_DATA = -5,
};

/* Implicit capabilities */
static struct osdp_pd_cap osdp_pd_cap[] = {
	{
		OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT,
		1, /* The PD supports the 16-bit CRC-16 mode */
		0, /* N/A */
	},
	{
		OSDP_PD_CAP_COMMUNICATION_SECURITY,
		1, /* (Bit-0) AES128 support */
		0, /* N/A */
	},
	{
		OSDP_PD_CAP_RECEIVE_BUFFERSIZE,
		BYTE_0(OSDP_PACKET_BUF_SIZE),
		BYTE_1(OSDP_PACKET_BUF_SIZE),
	},
	{
		OSDP_PD_CAP_OSDP_VERSION,
		2, /* SIA OSDP 2.2 */
		0, /* N/A */
	},
	{ -1, 0, 0 } /* Sentinel */
};

struct pd_event_node {
	queue_node_t node;
	struct osdp_event object;
};

static int pd_event_queue_init(struct osdp_pd *pd)
{
	if (slab_init(&pd->app_data.slab, sizeof(struct pd_event_node),
		      pd->app_data.slab_blob,
		      sizeof(pd->app_data.slab_blob)) < 0) {
		LOG_ERR("Failed to initialize command slab");
		return -1;
	}
	queue_init(&pd->event_queue);
	return 0;
}

static struct osdp_event *pd_event_alloc(struct osdp_pd *pd)
{
	struct pd_event_node *event = NULL;

	if (slab_alloc(&pd->app_data.slab, (void **)&event)) {
		LOG_ERR("Event slab allocation failed");
		return NULL;
	}
	memset(&event->object, 0, sizeof(event->object));
	return &event->object;
}

static void pd_event_free(struct osdp_pd *pd, struct osdp_event *event)
{
	struct pd_event_node *n;

	n = CONTAINER_OF(event, struct pd_event_node, object);
	slab_free(&pd->app_data.slab, n);
}

static void pd_event_enqueue(struct osdp_pd *pd, struct osdp_event *event)
{
	struct pd_event_node *n;

	n = CONTAINER_OF(event, struct pd_event_node, object);
	queue_enqueue(&pd->event_queue, &n->node);
}

static int pd_event_dequeue(struct osdp_pd *pd, struct osdp_event **event)
{
	struct pd_event_node *n;
	queue_node_t *node;

	if (queue_dequeue(&pd->event_queue, &node)) {
		return -1;
	}
	n = CONTAINER_OF(node, struct pd_event_node, node);
	*event = &n->object;
	return 0;
}

static int pd_translate_event(struct osdp_pd *pd, struct osdp_event *event)
{
	int reply_code = 0;

	switch (event->type) {
	case OSDP_EVENT_CARDREAD:
		if (event->cardread.format == OSDP_CARD_FMT_RAW_UNSPECIFIED ||
		    event->cardread.format == OSDP_CARD_FMT_RAW_WIEGAND) {
			reply_code = REPLY_RAW;
		} else if (event->cardread.format == OSDP_CARD_FMT_ASCII) {
			/**
			 * osdp_FMT was underspecified by SIA from get-go. It
			 * was marked for deprecation in v2.2.2.
			 *
			 * See: https://github.com/goToMain/libosdp/issues/206
			 */
			LOG_WRN("Event CardRead::format::OSDP_CARD_FMT_ASCII"
				" is deprecated. Ignoring");
		} else {
			LOG_ERR("Event: cardread; Error: unknown format");
			break;
		}
		break;
	case OSDP_EVENT_KEYPRESS:
		reply_code = REPLY_KEYPAD;
		break;
	case OSDP_EVENT_STATUS:
		switch(event->status.type) {
		case OSDP_STATUS_REPORT_INPUT:
			reply_code = REPLY_ISTATR;
			break;
		case OSDP_STATUS_REPORT_OUTPUT:
			reply_code = REPLY_OSTATR;
			break;
		case OSDP_STATUS_REPORT_LOCAL:
			reply_code = REPLY_LSTATR;
			break;
		case OSDP_STATUS_REPORT_REMOTE:
			reply_code = REPLY_RSTATR;
			break;
		}
		break;
	case OSDP_EVENT_MFGREP:
		reply_code = REPLY_MFGREP;
		break;
	default:
		LOG_ERR("Unknown event type %d", event->type);
		BUG();
	}
	if (reply_code == 0) {
		/* POLL command cannot fail even when there are errors here */
		return REPLY_ACK;
	}
	memcpy(pd->ephemeral_data, event, sizeof(struct osdp_event));
	return reply_code;
}

static bool do_command_callback(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	int ret = -1;

	if (pd->command_callback) {
		ret = pd->command_callback(pd->command_callback_arg, cmd);
	}
	if (ret != 0) {
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_RECORD;
		return false;
	}
	return true;
}

static int pd_cmd_cap_ok(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct osdp_pd_cap *cap = NULL;

	/* Validate the cmd_id against a PD capabilities where applicable */
	switch (pd->cmd_id) {
	case CMD_ISTAT:
		cap = &pd->cap[OSDP_PD_CAP_CONTACT_STATUS_MONITORING];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_OSTAT:
		cap = &pd->cap[OSDP_PD_CAP_OUTPUT_CONTROL];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_OUT:
		cap = &pd->cap[OSDP_PD_CAP_OUTPUT_CONTROL];
		if (!cmd || cap->compliance_level == 0 ||
		    cmd->output.output_no + 1 > cap->num_items) {
			break;
		}
		return 1;
	case CMD_LED:
		cap = &pd->cap[OSDP_PD_CAP_READER_LED_CONTROL];
		if (!cmd || cap->compliance_level == 0 ||
		    cmd->led.led_number + 1 > cap->num_items) {
			break;
		}
		return 1;
	case CMD_BUZ:
		cap = &pd->cap[OSDP_PD_CAP_READER_AUDIBLE_OUTPUT];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_TEXT:
		cap = &pd->cap[OSDP_PD_CAP_READER_TEXT_OUTPUT];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_CHLNG:
	case CMD_SCRYPT:
	case CMD_KEYSET:
		cap = &pd->cap[OSDP_PD_CAP_COMMUNICATION_SECURITY];
		if (cap->compliance_level == 0) {
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_UNSUP;
			return 0;
		}
		return 1;
	}

	pd->reply_id = REPLY_NAK;
	pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_UNKNOWN;
	LOG_ERR("PD is not capable of handling CMD(%02x); ", pd->cmd_id);
	return 0;
}

static void pd_stage_event_mfgrep(struct osdp_pd *pd, struct osdp_cmd_mfg *cmd)
{
	struct osdp_event ev;

	ev.type = OSDP_EVENT_MFGREP;
	ev.flags = 0;

	ev.mfgrep.length = cmd->length;
	ev.mfgrep.vendor_code = cmd->vendor_code;
	memcpy(ev.mfgrep.data, cmd->data, cmd->length);

	memcpy(pd->ephemeral_data, &ev, sizeof(ev));
}

static int pd_decode_command(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int i, ret = OSDP_PD_ERR_GENERIC, pos = 0;
	struct osdp_cmd cmd;
	struct osdp_event *event;

	pd->reply_id = REPLY_NAK;
	pd->ephemeral_data[0] = OSDP_PD_NAK_RECORD;
	pd->cmd_id = cmd.id = buf[pos++];
	len--;

	if (is_enforce_secure(pd) && !sc_is_active(pd)) {
		/**
		 * Only CMD_ID, CMD_CAP and SC handshake commands (CMD_CHLNG
		 * and CMD_SCRYPT) are allowed when SC is inactive and
		 * ENFORCE_SECURE was requested.
		 */
		if (pd->cmd_id != CMD_ID && pd->cmd_id != CMD_CAP &&
		    pd->cmd_id != CMD_CHLNG && pd->cmd_id != CMD_SCRYPT) {
			LOG_ERR("CMD: %s(%02x) not allowed due to ENFORCE_SECURE",
				osdp_cmd_name(pd->cmd_id), pd->cmd_id);
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_PD_ERR_REPLY;
		}
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
		if (len != CMD_POLL_DATA_LEN) {
			break;
		}
		/* Check if we have external events in the queue */
		if (pd_event_dequeue(pd, &event) == 0) {
			ret = pd_translate_event(pd, event);
			pd->reply_id = ret;
			pd_event_free(pd, event);
		} else {
			pd->reply_id = REPLY_ACK;
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_LSTAT:
		if (len != CMD_LSTAT_DATA_LEN) {
			break;
		}
		cmd.id = OSDP_CMD_STATUS;
		cmd.status.type = OSDP_STATUS_REPORT_LOCAL;
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		event = (struct osdp_event *)pd->ephemeral_data;
		event->type = OSDP_EVENT_STATUS;
		memcpy(&event->status, &cmd.status, sizeof(cmd.status));
		pd->reply_id = REPLY_LSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ISTAT:
		if (len != CMD_ISTAT_DATA_LEN) {
			break;
		}
		if (!pd_cmd_cap_ok(pd, NULL)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		cmd.id = OSDP_CMD_STATUS;
		cmd.status.type = OSDP_STATUS_REPORT_INPUT;
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		event = (struct osdp_event *)pd->ephemeral_data;
		event->type = OSDP_EVENT_STATUS;
		memcpy(&event->status, &cmd.status, sizeof(cmd.status));
		pd->reply_id = REPLY_ISTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_OSTAT:
		if (len != CMD_OSTAT_DATA_LEN) {
			break;
		}
		if (!pd_cmd_cap_ok(pd, NULL)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		cmd.id = OSDP_CMD_STATUS;
		cmd.status.type = OSDP_STATUS_REPORT_OUTPUT;
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		event = (struct osdp_event *)pd->ephemeral_data;
		event->type = OSDP_EVENT_STATUS;
		memcpy(&event->status, &cmd.status, sizeof(cmd.status));
		pd->reply_id = REPLY_OSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_RSTAT:
		if (len != CMD_RSTAT_DATA_LEN) {
			break;
		}
		cmd.id = OSDP_CMD_STATUS;
		cmd.status.type = OSDP_STATUS_REPORT_REMOTE;
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		event = (struct osdp_event *)pd->ephemeral_data;
		event->type = OSDP_EVENT_STATUS;
		memcpy(&event->status, &cmd.status, sizeof(cmd.status));
		pd->reply_id = REPLY_RSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ID:
		if (len != CMD_ID_DATA_LEN) {
			break;
		}
		pos++;		/* Skip reply type info. */
		pd->reply_id = REPLY_PDID;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_CAP:
		if (len != CMD_CAP_DATA_LEN) {
			break;
		}
		pos++;		/* Skip reply type info. */
		pd->reply_id = REPLY_PDCAP;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_OUT:
		if ((len % CMD_OUT_DATA_LEN) != 0) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		for (i = 0; i < len / CMD_OUT_DATA_LEN; i++) {
			cmd.id = OSDP_CMD_OUTPUT;
			cmd.output.output_no = buf[pos++];
			cmd.output.control_code = buf[pos++];
			cmd.output.timer_count = buf[pos++];
			cmd.output.timer_count |= buf[pos++] << 8;
			if (!pd_cmd_cap_ok(pd, &cmd)) {
				break;
			}
			if (!do_command_callback(pd, &cmd)) {
				break;
			}
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_LED:
		if ((len % CMD_LED_DATA_LEN) != 0) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		for (i = 0; i < len / CMD_LED_DATA_LEN; i++) {
			cmd.id = OSDP_CMD_LED;
			cmd.led.reader = buf[pos++];
			cmd.led.led_number = buf[pos++];

			cmd.led.temporary.control_code = buf[pos++];
			cmd.led.temporary.on_count = buf[pos++];
			cmd.led.temporary.off_count = buf[pos++];
			cmd.led.temporary.on_color = buf[pos++];
			cmd.led.temporary.off_color = buf[pos++];
			cmd.led.temporary.timer_count = buf[pos++];
			cmd.led.temporary.timer_count |= buf[pos++] << 8;

			cmd.led.permanent.control_code = buf[pos++];
			cmd.led.permanent.on_count = buf[pos++];
			cmd.led.permanent.off_count = buf[pos++];
			cmd.led.permanent.on_color = buf[pos++];
			cmd.led.permanent.off_color = buf[pos++];
			if (!pd_cmd_cap_ok(pd, &cmd)) {
				break;
			}
			if (!do_command_callback(pd, &cmd)) {
				break;
			}
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_BUZ:
		if ((len % CMD_BUZ_DATA_LEN) != 0) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		for (i = 0; i < len / CMD_BUZ_DATA_LEN; i++) {
			cmd.id = OSDP_CMD_BUZZER;
			cmd.buzzer.reader = buf[pos++];
			cmd.buzzer.control_code = buf[pos++];
			cmd.buzzer.on_count = buf[pos++];
			cmd.buzzer.off_count = buf[pos++];
			cmd.buzzer.rep_count = buf[pos++];
			if (!pd_cmd_cap_ok(pd, &cmd)) {
				break;
			}
			if (!do_command_callback(pd, &cmd)) {
				break;
			}
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_TEXT:
		if (len < CMD_TEXT_DATA_LEN) {
			break;
		}
		cmd.id = OSDP_CMD_TEXT;
		cmd.text.reader = buf[pos++];
		cmd.text.control_code = buf[pos++];
		cmd.text.temp_time = buf[pos++];
		cmd.text.offset_row = buf[pos++];
		cmd.text.offset_col = buf[pos++];
		cmd.text.length = buf[pos++];
		if (cmd.text.length > OSDP_CMD_TEXT_MAX_LEN ||
		    ((len - CMD_TEXT_DATA_LEN) < cmd.text.length) ||
		    cmd.text.length > OSDP_CMD_TEXT_MAX_LEN) {
			break;
		}
		memcpy(cmd.text.data, buf + pos, cmd.text.length);
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, &cmd)) {
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			break;
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_COMSET:
		if (len != CMD_COMSET_DATA_LEN) {
			break;
		}
		cmd.id = OSDP_CMD_COMSET;
		cmd.comset.address = buf[pos++];
		cmd.comset.baud_rate = buf[pos++];
		cmd.comset.baud_rate |= buf[pos++] << 8;
		cmd.comset.baud_rate |= buf[pos++] << 16;
		cmd.comset.baud_rate |= buf[pos++] << 24;
		if (cmd.comset.address >= 0x7F) {
			LOG_ERR("COMSET Failed! command discarded");
			cmd.comset.address = pd->address;
			cmd.comset.baud_rate = pd->baud_rate;
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		memcpy(pd->ephemeral_data, &cmd, sizeof(struct osdp_cmd));
		pd->reply_id = REPLY_COM;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_MFG:
		if (len < CMD_MFG_DATA_LEN) {
			break;
		}
		cmd.id = OSDP_CMD_MFG;
		cmd.mfg.vendor_code = buf[pos++]; /* vendor_code */
		cmd.mfg.vendor_code |= buf[pos++] << 8;
		cmd.mfg.vendor_code |= buf[pos++] << 16;
		cmd.mfg.length = len - CMD_MFG_DATA_LEN;
		if (cmd.mfg.length > OSDP_CMD_MFG_MAX_DATALEN) {
			LOG_ERR("cmd length error");
			break;
		}
		for (i = 0; i < cmd.mfg.length; i++) {
			cmd.mfg.data[i] = buf[pos++];
		}

		ret = 0;
		if (pd->command_callback) {
			ret = pd->command_callback(pd->command_callback_arg, &cmd);
		}
		if (ret < 0) { /* Callback failed */
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_RECORD;
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		if (ret > 0) { /* App wants to send a REPLY_MFGREP to the CP */
			pd_stage_event_mfgrep(pd, &cmd.mfg);
			pd->reply_id = REPLY_MFGREP;
		} else {
			pd->reply_id = REPLY_ACK;
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ACURXSIZE:
		if (len < CMD_ACURXSIZE_DATA_LEN) {
			break;
		}
		pd->peer_rx_size = buf[pos] | (buf[pos + 1] << 8);
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_KEEPACTIVE:
		if (len < CMD_KEEPACTIVE_DATA_LEN) {
			break;
		}
		pd->sc_tstamp += buf[pos] | (buf[pos + 1] << 8);
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ABORT:
		if (len != CMD_ABORT_DATA_LEN) {
			break;
		}
		osdp_file_tx_abort(pd);
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_FILETRANSFER:
		ret = osdp_file_cmd_tx_decode(pd, buf + pos, len);
		if (ret == 0) {
			ret = OSDP_PD_ERR_NONE;
			pd->reply_id = REPLY_FTSTAT;
			break;
		}
		break;
	case CMD_KEYSET:
		if (len != CMD_KEYSET_DATA_LEN) {
			break;
		}
		/* only key_type == 1 (SCBK) and key_len == 16 is supported */
		if (buf[pos] != 1 || buf[pos + 1] != 16) {
			LOG_ERR("Keyset invalid len/type: %d/%d",
				buf[pos], buf[pos + 1]);
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		if (!sc_is_active(pd)) {
			LOG_ERR("Keyset with SC inactive");
			break;
		}
		if (!pd->command_callback) {
			LOG_ERR("Keyset not permitted without setting a command"
				" callback; rejecting new KEY");
			break;
		}
		cmd.id = OSDP_CMD_KEYSET;
		cmd.keyset.type = buf[pos++];
		cmd.keyset.length = buf[pos++];
		memcpy(cmd.keyset.data, buf + pos, 16);
		if (!do_command_callback(pd, &cmd)) {
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			LOG_ERR("Keyset with SC inactive");
			break;
		}
		ret = OSDP_PD_ERR_NONE;
		pd->reply_id = REPLY_ACK;
		memcpy(pd->ephemeral_data, cmd.keyset.data, 16);
		break;
	case CMD_CHLNG:
		if (len != CMD_CHLNG_DATA_LEN) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		sc_deactivate(pd);
		osdp_sc_setup(pd);
		memcpy(pd->sc.cp_random, buf + pos, 8);
		pd->reply_id = REPLY_CCRYPT;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_SCRYPT:
		if (len != CMD_SCRYPT_DATA_LEN) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		if (sc_is_active(pd)) {
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			LOG_EM("Out of order CMD_SCRYPT; has CP gone rogue?");
			break;
		}
		memcpy(pd->sc.cp_cryptogram, buf + pos, CMD_SCRYPT_DATA_LEN);
		pd->reply_id = REPLY_RMAC_I;
		ret = OSDP_PD_ERR_NONE;
		break;
	default:
		LOG_ERR("Unknown CMD(%02x)", pd->cmd_id);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_UNKNOWN;
		return OSDP_PD_ERR_REPLY;
	}

	if (ret == OSDP_PD_ERR_GENERIC) {
		LOG_ERR("Failed to decode command: CMD(%02x) Len:%d ret:%d",
			pd->cmd_id, len, ret);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_LEN;
		ret = OSDP_PD_ERR_REPLY;
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG("CMD: %s(%02x) REPLY: %s(%02x)",
			osdp_cmd_name(pd->cmd_id), pd->cmd_id,
			osdp_reply_name(pd->reply_id), pd->reply_id);
	}

	return ret;
}

static inline void assert_buf_len(int need, int have)
{
	__ASSERT(need < have, "OOM at build command: need:%d have:%d",
		 need, have);
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
static int pd_build_reply(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int ret = OSDP_PD_ERR_GENERIC;
	int i, len = 0;
	struct osdp_cmd *cmd;
	struct osdp_event *event;
	int data_off = osdp_phy_packet_get_data_offset(pd, buf);
	uint8_t *smb = osdp_phy_packet_get_smb(pd, buf);

	buf += data_off;
	max_len -= data_off;

	switch (pd->reply_id) {
	case REPLY_ACK:
		assert_buf_len(REPLY_ACK_LEN, max_len);
		buf[len++] = pd->reply_id;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_PDID:
		assert_buf_len(REPLY_PDID_LEN, max_len);
		buf[len++] = pd->reply_id;

		buf[len++] = BYTE_0(pd->id.vendor_code);
		buf[len++] = BYTE_1(pd->id.vendor_code);
		buf[len++] = BYTE_2(pd->id.vendor_code);

		buf[len++] = pd->id.model;
		buf[len++] = pd->id.version;

		buf[len++] = BYTE_0(pd->id.serial_number);
		buf[len++] = BYTE_1(pd->id.serial_number);
		buf[len++] = BYTE_2(pd->id.serial_number);
		buf[len++] = BYTE_3(pd->id.serial_number);

		buf[len++] = BYTE_2(pd->id.firmware_version);
		buf[len++] = BYTE_1(pd->id.firmware_version);
		buf[len++] = BYTE_0(pd->id.firmware_version);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_PDCAP:
		assert_buf_len(REPLY_PDCAP_LEN, max_len);
		buf[len++] = pd->reply_id;
		for (i = 1; i < OSDP_PD_CAP_SENTINEL; i++) {
			if (pd->cap[i].function_code != i) {
				continue;
			}
			if (max_len < REPLY_PDCAP_ENTITY_LEN) {
				LOG_ERR("Out of buffer space!");
				break;
			}
			buf[len++] = i;
			buf[len++] = pd->cap[i].compliance_level;
			buf[len++] = pd->cap[i].num_items;
			max_len -= REPLY_PDCAP_ENTITY_LEN;
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_OSTATR: {
		event = (struct osdp_event *)pd->ephemeral_data;
		int n = pd->cap[OSDP_PD_CAP_OUTPUT_CONTROL].num_items;
		if (event->status.nr_entries != n) {
			break;
		}
		assert_buf_len(n + 1, max_len);
		buf[len++] = pd->reply_id;
		for (i = 0; i < n; i++) {
			buf[len++] = event->status.report[i];
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	}
	case REPLY_ISTATR: {
		event = (struct osdp_event *)pd->ephemeral_data;
		int n = pd->cap[OSDP_PD_CAP_CONTACT_STATUS_MONITORING].num_items;
		if (event->status.nr_entries != n) {
			break;
		}
		assert_buf_len(n + 1, max_len);
		buf[len++] = pd->reply_id;
		for (i = 0; i < n; i++) {
			buf[len++] = event->status.report[i];
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	}
	case REPLY_LSTATR:
		assert_buf_len(REPLY_LSTATR_LEN, max_len);
		event = (struct osdp_event *)pd->ephemeral_data;
		buf[len++] = pd->reply_id;
		buf[len++] = event->status.report[0]; // tamper
		buf[len++] = event->status.report[1]; // power
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RSTATR:
		assert_buf_len(REPLY_RSTATR_LEN, max_len);
		event = (struct osdp_event *)pd->ephemeral_data;
		buf[len++] = pd->reply_id;
		buf[len++] = event->status.report[0]; // power
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_KEYPAD:
		event = (struct osdp_event *)pd->ephemeral_data;
		assert_buf_len(REPLY_KEYPAD_LEN + event->keypress.length, max_len);
		buf[len++] = pd->reply_id;
		buf[len++] = (uint8_t)event->keypress.reader_no;
		buf[len++] = (uint8_t)event->keypress.length;
		memcpy(buf + len, event->keypress.data, event->keypress.length);
		len += event->keypress.length;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RAW: {
		int len_bytes;

		event = (struct osdp_event *)pd->ephemeral_data;
		len_bytes = (event->cardread.length + 7) / 8;
		assert_buf_len(REPLY_RAW_LEN + len_bytes, max_len);
		buf[len++] = pd->reply_id;
		buf[len++] = (uint8_t)event->cardread.reader_no;
		buf[len++] = (uint8_t)event->cardread.format;
		buf[len++] = BYTE_0(event->cardread.length);
		buf[len++] = BYTE_1(event->cardread.length);
		memcpy(buf + len, event->cardread.data, len_bytes);
		len += len_bytes;
		ret = OSDP_PD_ERR_NONE;
		break;
	}
	case REPLY_COM:
		assert_buf_len(REPLY_COM_LEN, max_len);
		/**
		 * If COMSET succeeds, the PD must reply with the old params and
		 * then switch to the new params from then then on. We have the
		 * new params in the commands struct that we just enqueued so
		 * we can peek at tail of command queue and set that to
		 * pd->addr/pd->baud_rate.
		 */
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->reply_id;
		buf[len++] = cmd->comset.address;
		buf[len++] = BYTE_0(cmd->comset.baud_rate);
		buf[len++] = BYTE_1(cmd->comset.baud_rate);
		buf[len++] = BYTE_2(cmd->comset.baud_rate);
		buf[len++] = BYTE_3(cmd->comset.baud_rate);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_NAK:
		assert_buf_len(REPLY_NAK_LEN, max_len);
		buf[len++] = pd->reply_id;
		buf[len++] = pd->ephemeral_data[0];
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_MFGREP:
		event = (struct osdp_event *)pd->ephemeral_data;
		assert_buf_len(REPLY_MFGREP_LEN + event->mfgrep.length, max_len);
		buf[len++] = pd->reply_id;
		buf[len++] = BYTE_0(event->mfgrep.vendor_code);
		buf[len++] = BYTE_1(event->mfgrep.vendor_code);
		buf[len++] = BYTE_2(event->mfgrep.vendor_code);
		memcpy(buf + len, event->mfgrep.data, event->mfgrep.length);
		len += event->mfgrep.length;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_FTSTAT:
		buf[len++] = pd->reply_id;
		ret = osdp_file_cmd_stat_build(pd, buf + len, max_len);
		if (ret <= 0) {
			break;
		}
		len += ret;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_CCRYPT:
		if (smb == NULL) {
			break;
		}
		assert_buf_len(REPLY_CCRYPT_LEN, max_len);
		osdp_fill_random(pd->sc.pd_random, 8);
		osdp_compute_session_keys(pd);
		osdp_compute_pd_cryptogram(pd);
		buf[len++] = pd->reply_id;
		memcpy(buf + len, pd->sc.pd_client_uid, 8);
		memcpy(buf + len + 8, pd->sc.pd_random, 8);
		memcpy(buf + len + 16, pd->sc.pd_cryptogram, 16);
		len += 32;
		smb[0] = 3;      /* length */
		smb[1] = SCS_12; /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RMAC_I:
		if (smb == NULL) {
			break;
		}
		assert_buf_len(REPLY_RMAC_I_LEN, max_len);
		osdp_compute_rmac_i(pd);
		buf[len++] = pd->reply_id;
		memcpy(buf + len, pd->sc.r_mac, 16);
		len += 16;
		smb[0] = 3;       /* length */
		smb[1] = SCS_14;  /* type */
		if (osdp_verify_cp_cryptogram(pd) == 0) {
			smb[2] = 1;  /* CP auth succeeded */
			sc_activate(pd);
			pd->sc_tstamp = osdp_millis_now();
			if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
				LOG_WRN("SC Active with SCBK-D");
			} else {
				LOG_INF("SC Active");
			}
		} else {
			smb[2] = 0;  /* CP auth failed */
			LOG_WRN("failed to verify CP_crypt");
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	default: BUG();
	}

	if (smb && (smb[1] > SCS_14) && sc_is_active(pd)) {
		smb[0] = 2; /* length */
		smb[1] = (len > 1) ? SCS_18 : SCS_16;
	}

	if (ret != 0) {
		/* catch all errors and report it as a RECORD error to CP */
		LOG_ERR("Failed to build REPLY: %s(%02x); Sending NAK instead!",
			osdp_reply_name(pd->reply_id), pd->reply_id);
		assert_buf_len(REPLY_NAK_LEN, max_len);
		buf[0] = REPLY_NAK;
		buf[1] = OSDP_PD_NAK_RECORD;
		len = 2;
	}

	return len;
}

static int pd_send_reply(struct osdp_pd *pd)
{
	int ret, packet_buf_size = get_tx_buf_size(pd);

	/* init packet buf with header */
	ret = osdp_phy_packet_init(pd, pd->packet_buf, packet_buf_size);
	if (ret < 0) {
		return OSDP_PD_ERR_GENERIC;
	}
	pd->packet_buf_len = ret;

	/* fill reply data */
	ret = pd_build_reply(pd, pd->packet_buf, packet_buf_size);
	if (ret <= 0) {
		return OSDP_PD_ERR_GENERIC;
	}
	pd->packet_buf_len += ret;

	ret = osdp_phy_send_packet(pd, pd->packet_buf, pd->packet_buf_len,
				   packet_buf_size);
	if (ret < 0) {
		return OSDP_PD_ERR_GENERIC;
	}

	return OSDP_PD_ERR_NONE;
}

static int pd_receive_and_process_command(struct osdp_pd *pd)
{
	int err, len;
	uint8_t *buf;

	err = osdp_phy_check_packet(pd);

	/* Translate phy error codes to PD errors */
	switch (err) {
	case OSDP_ERR_PKT_NONE:
		break;
	case OSDP_ERR_PKT_NACK:
		return OSDP_PD_ERR_REPLY;
	case OSDP_ERR_PKT_NO_DATA:
		return OSDP_PD_ERR_NO_DATA;
	case OSDP_ERR_PKT_WAIT:
		return OSDP_PD_ERR_WAIT;
	case OSDP_ERR_PKT_SKIP:
		osdp_phy_state_reset(pd, false);
		return OSDP_PD_ERR_IGNORE;
	case OSDP_ERR_PKT_FMT:
		return OSDP_PD_ERR_GENERIC;
	default:
		return err; /* propagate other errors as-is */
	}

	len = osdp_phy_decode_packet(pd, &buf);
	if (len <= 0) {
		if (len == OSDP_ERR_PKT_NACK) {
			return OSDP_PD_ERR_REPLY; /* Send a NAK */
		}
		return OSDP_PD_ERR_GENERIC; /* fatal errors */
	}

	return pd_decode_command(pd, buf, len);
}

static inline void pd_error_reset(struct osdp_pd *pd)
{
	sc_deactivate(pd);
	osdp_phy_state_reset(pd, false);
}

static void osdp_pd_update(struct osdp_pd *pd)
{
	int ret;
	struct osdp_cmd *cmd;

	/**
	 * If secure channel is established, we need to make sure that
	 * the session is valid before accepting a command.
	 */
	if (sc_is_active(pd) &&
	    osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_TIMEOUT_MS) {
		LOG_INF("PD SC session timeout!");
		sc_deactivate(pd);
	}

	ret = pd_receive_and_process_command(pd);

	if (ret == OSDP_PD_ERR_IGNORE || ret == OSDP_PD_ERR_NO_DATA) {
		return;
	}

	if (ret == OSDP_PD_ERR_WAIT &&
	    osdp_millis_since(pd->tstamp) < OSDP_RESP_TOUT_MS) {
		return;
	}

	if (ret != OSDP_PD_ERR_NONE && ret != OSDP_PD_ERR_REPLY) {
		LOG_ERR("CMD receive error/timeout - err:%d", ret);
		pd_error_reset(pd);
		return;
	}

	if (ret == OSDP_PD_ERR_NONE && sc_is_active(pd)) {
		pd->sc_tstamp = osdp_millis_now();
	}

	ret = pd_send_reply(pd);
	if (ret == OSDP_PD_ERR_NONE) {
		if (pd->cmd_id == CMD_KEYSET && pd->reply_id == REPLY_ACK) {
			memcpy(pd->sc.scbk, pd->ephemeral_data, 16);
			CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			CLEAR_FLAG(pd, OSDP_FLAG_INSTALL_MODE);
			sc_deactivate(pd);
		} else if (pd->cmd_id == CMD_COMSET &&
			   pd->reply_id == REPLY_COM) {
			/* COMSET command succeeded all the way:
			 *
			 * - CP requested the change (with OSDP_CMD_COMSET)
			 * - PD app ack-ed this change (but didn't commit
			 *   the change to it's non-volatile storage)
			 * - CP was notified that the command succeeded. So
			 *   it should have switched to the new settings
			 *
			 *  Now we must notify the PD app so it can actually
			 *  switch the channel speed, reset any other state
			 *  it held and commit this change to non-volatile
			 *  storage.
			 */
			cmd = (struct osdp_cmd *)pd->ephemeral_data;
			cmd->id = OSDP_CMD_COMSET_DONE;
			do_command_callback(pd, cmd);
			pd->address = (int)cmd->comset.address;
			pd->baud_rate = (int)cmd->comset.baud_rate;
			LOG_INF("COMSET Succeeded! New PD-Addr: %d; Baud: %d",
				pd->address, pd->baud_rate);
		}
		osdp_phy_progress_sequence(pd);
	} else {
		/**
		 * PD received and decoded a valid command from CP but failed to
		 * send the intended response?? This should not happen; but if
		 * it did, we cannot do anything about it, just complain about
		 * it and limp back home.
		 */
		LOG_EM("REPLY send failed! CP may be waiting..");
	}
	osdp_phy_state_reset(pd, false);
}

static void osdp_pd_set_attributes(struct osdp_pd *pd,
				   const struct osdp_pd_cap *cap,
				   const struct osdp_pd_id *id)
{
	int fc;

	while (cap && ((fc = cap->function_code) > 0)) {
		if (fc >= OSDP_PD_CAP_SENTINEL) {
			break;
		}
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}
	if (id != NULL) {
		memcpy(&pd->id, id, sizeof(struct osdp_pd_id));
	}
}

/* --- Exported Methods --- */

osdp_t *osdp_pd_setup(const osdp_pd_info_t *info)
{
	struct osdp_pd *pd;
	struct osdp *ctx;
	char name[16] = {0};

	assert(info);

#ifndef OPT_OSDP_STATIC_PD
	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_PRINT("Failed to allocate osdp context");
		return NULL;
	}

	ctx->pd = calloc(1, sizeof(struct osdp_pd));
	if (ctx->pd == NULL) {
		LOG_PRINT("Failed to allocate osdp_pd context");
		goto error;
	}
#else
	static struct osdp g_osdp_ctx;
	static struct osdp_pd g_osdp_pd_ctx;

	ctx = &g_osdp_ctx;
	ctx->pd = &g_osdp_pd_ctx;
#endif

	input_check_init(ctx);
	ctx->_num_pd = 1;

	SET_CURRENT_PD(ctx, 0);
	pd = osdp_to_pd(ctx, 0);

	pd->osdp_ctx = ctx;
	pd->idx = 0;
	if (info->name) {
		strncpy(pd->name, info->name, OSDP_PD_NAME_MAXLEN - 1);
	} else {
		snprintf(pd->name, OSDP_PD_NAME_MAXLEN, "PD-%d", info->address);
	}
	pd->baud_rate = info->baud_rate;
	pd->address = info->address;
	pd->flags = info->flags;
	pd->seq_number = -1;
	memcpy(&pd->channel, &info->channel, sizeof(struct osdp_channel));

	logger_get_default(&pd->logger);
	snprintf(name, sizeof(name), "OSDP: PD-%d", pd->address);
	logger_set_name(&pd->logger, name);

	if (pd_event_queue_init(pd)) {
		goto error;
	}

	if (info->scbk == NULL) {
		if (is_enforce_secure(pd)) {
			LOG_ERR("SCBK must be provided in ENFORCE_SECURE");
			goto error;
		}
		LOG_WRN("SCBK not provided. PD is in INSTALL_MODE");
		SET_FLAG(pd, OSDP_FLAG_INSTALL_MODE);
	} else {
		memcpy(pd->sc.scbk, info->scbk, 16);
	}
	SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
	if (IS_ENABLED(OPT_OSDP_SKIP_MARK_BYTE)) {
		SET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK);
	}
	osdp_pd_set_attributes(pd, info->cap, &info->id);
	osdp_pd_set_attributes(pd, osdp_pd_cap, NULL);

	SET_FLAG(pd, PD_FLAG_PD_MODE); /* used in checks in phy */

	if (is_capture_enabled(pd)) {
		osdp_packet_capture_init(pd);
	}

	LOG_PRINT("PD Setup complete; LibOSDP-%s %s",
		  osdp_get_version(), osdp_get_source_info());

	return (osdp_t *)ctx;
error:
	osdp_pd_teardown((osdp_t *)ctx);
	return NULL;
}

void osdp_pd_teardown(osdp_t *ctx)
{
	assert(ctx);
	struct osdp_pd *pd = osdp_to_pd(ctx, 0);

	if (is_capture_enabled(pd)) {
		osdp_packet_capture_finish(pd);
	}

	if (pd->channel.close) {
		pd->channel.close(pd->channel.data);
	}

#ifndef OPT_OSDP_STATIC_PD
	safe_free(pd->file);
	safe_free(pd);
	safe_free(ctx);
#endif
}

void osdp_pd_refresh(osdp_t *ctx)
{
	input_check(ctx);
	struct osdp_pd *pd = GET_CURRENT_PD(ctx);

	osdp_pd_update(pd);
}

void osdp_pd_set_capabilities(osdp_t *ctx, const struct osdp_pd_cap *cap)
{
	input_check(ctx);
	struct osdp_pd *pd = GET_CURRENT_PD(ctx);

	osdp_pd_set_attributes(pd, cap, NULL);
}

void osdp_pd_set_command_callback(osdp_t *ctx, pd_command_callback_t cb,
				  void *arg)
{
	input_check(ctx);
	struct osdp_pd *pd = GET_CURRENT_PD(ctx);

	pd->command_callback_arg = arg;
	pd->command_callback = cb;
}

int osdp_pd_submit_event(osdp_t *ctx, const struct osdp_event *event)
{
	input_check(ctx);
	struct osdp_event *ev;
	struct osdp_pd *pd = GET_CURRENT_PD(ctx);

	if (event->type <= 0 ||
	    event->type >= OSDP_EVENT_SENTINEL) {
		return -1;
	}

	ev = pd_event_alloc(pd);
	if (ev == NULL) {
		return -1;
	}

	memcpy(ev, event, sizeof(struct osdp_event));
	pd_event_enqueue(pd, ev);
	return 0;
}

int osdp_pd_notify_event(osdp_t *ctx, const struct osdp_event *event)
{
	return osdp_pd_submit_event(ctx, event);
}

int osdp_pd_flush_events(osdp_t *ctx)
{
	input_check(ctx);
	int count = 0;
	struct osdp_event *ev;
	struct osdp_pd *pd = GET_CURRENT_PD(ctx);

	while (pd_event_dequeue(pd, &ev) == 0) {
		pd_event_free(pd, ev);
		count++;
	}

	return count;
}
