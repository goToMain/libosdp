/*
 * Copyright (c) 2019-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "osdp_common.h"

#define OSDP_PKT_MARK                  0xFF
#define OSDP_PKT_SOM                   0x53
#define PKT_CONTROL_SQN                0x03
#define PKT_CONTROL_CRC                0x04
#define PKT_CONTROL_SCB                0x08

struct osdp_packet_header {
	uint8_t som;
	uint8_t pd_address;
	uint8_t len_lsb;
	uint8_t len_msb;
	uint8_t control;
	uint8_t data[];
} __packed;

static inline bool packet_has_mark(struct osdp_pd *pd)
{
	return ISSET_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
}

static int osdp_channel_send(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int sent, total_sent = 0;

	/* flush rx to remove any invalid data. */
	if (pd->channel.flush) {
		pd->channel.flush(pd->channel.data);
	}

	do { /* send can block; so be greedy */
		sent = pd->channel.send(pd->channel.data,
					buf + total_sent, len - total_sent);
		if (sent <= 0) {
			break;
		}
		total_sent += sent;
	} while (total_sent < len);

	return total_sent;
}

static int osdp_channel_receive(struct osdp_pd *pd)
{
	uint8_t buf[64];
	int recv, total_recv = 0;

#ifdef UNIT_TESTING
	/**
	 * Some unit tests don't define pd->channel.recv and directly fill
	 * pd->packet_buf to test if everything else work correctly.
	 */
	if (!pd->channel.recv) {
		return 0;
	}
#endif

	do {
		recv = pd->channel.recv(pd->channel.data, buf, sizeof(buf));
		if (recv <= 0) {
			break;
		}
		if (osdp_rb_push_buf(&pd->rx_rb, buf, recv) != recv) {
			LOG_EM("RX ring buffer overflow!");
			return -1;
		}
		total_recv += recv;
	} while (recv == sizeof(buf));

	return total_recv;
}

uint8_t osdp_compute_checksum(uint8_t *msg, int length)
{
	uint8_t checksum = 0;
	int i, whole_checksum;

	whole_checksum = 0;
	for (i = 0; i < length; i++) {
		whole_checksum += msg[i];
		checksum = ~(0xff & whole_checksum) + 1;
	}
	return checksum;
}

static int osdp_phy_get_seq_number(struct osdp_pd *pd, int do_inc)
{
	/* pd->seq_num is set to -1 to reset phy cmd state */
	if (do_inc) {
		pd->seq_number += 1;
		if (pd->seq_number > 3) {
			pd->seq_number = 1;
		}
	}
	return pd->seq_number & PKT_CONTROL_SQN;
}

int osdp_phy_packet_get_data_offset(struct osdp_pd *pd, const uint8_t *buf)
{
	int sb_len = 0, mark_byte_len = 0;
	struct osdp_packet_header *pkt;

	ARG_UNUSED(pd);
	if (packet_has_mark(pd)) {
		mark_byte_len = 1;
		buf += 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->control & PKT_CONTROL_SCB) {
		sb_len = pkt->data[0];
	}
	return mark_byte_len + sizeof(struct osdp_packet_header) + sb_len;
}

uint8_t *osdp_phy_packet_get_smb(struct osdp_pd *pd, const uint8_t *buf)
{
	struct osdp_packet_header *pkt;

	ARG_UNUSED(pd);
	pkt = (struct osdp_packet_header *)(buf + packet_has_mark(pd));
	if (pkt->control & PKT_CONTROL_SCB) {
		return pkt->data;
	}
	return NULL;
}

int osdp_phy_in_sc_handshake(int is_reply, int id)
{
	if (is_reply) {
		return (id == REPLY_CCRYPT || id == REPLY_RMAC_I);
	} else {
		return (id == CMD_CHLNG || id == CMD_SCRYPT);
	}
}

int osdp_phy_packet_init(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int exp_len, id, scb_len = 0;
	struct osdp_packet_header *pkt;

	exp_len = sizeof(struct osdp_packet_header) + 64; /* 64 is estimated */
	if (max_len < exp_len) {
		LOG_ERR("packet_init: out of space! CMD: %02x", pd->cmd_id);
		return OSDP_ERR_PKT_FMT;
	}

	/**
	 * In PD mode just follow what we received from CP. In CP mode, as we
	 * initiate the transaction, choose based on CONFIG_OSDP_SKIP_MARK_BYTE.
	 */
	if ((is_pd_mode(pd) && packet_has_mark(pd)) ||
	    (is_cp_mode(pd) && !ISSET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK))) {
		buf[0] = OSDP_PKT_MARK;
		buf++;
		SET_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
	}

	/* Fill packet header */
	pkt = (struct osdp_packet_header *)buf;
	pkt->som = OSDP_PKT_SOM;
	pkt->pd_address = pd->address & 0x7F;	/* Use only the lower 7 bits */
	if (is_pd_mode(pd)) {
		/* PD must reply with MSB of it's address set */
		if (ISSET_FLAG(pd, PD_FLAG_PKT_BROADCAST)) {
			pkt->pd_address = 0x7F; /* made 0xFF below */
			CLEAR_FLAG(pd, PD_FLAG_PKT_BROADCAST);
		}
		pkt->pd_address |= 0x80;
		id = pd->reply_id;
	} else {
		id = pd->cmd_id;
	}
	pkt->control = osdp_phy_get_seq_number(pd, is_cp_mode(pd));
	pkt->control |= PKT_CONTROL_CRC;

	if (sc_is_active(pd)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = scb_len = 2;
		pkt->data[1] = SCS_15;
	} else if (osdp_phy_in_sc_handshake(is_pd_mode(pd), id)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = scb_len = 3;
		pkt->data[1] = SCS_11;
	}

	return (packet_has_mark(pd) +
	        sizeof(struct osdp_packet_header) + scb_len);
}

static int osdp_phy_packet_finalize(struct osdp_pd *pd, uint8_t *buf,
				    int len, int max_len)
{
	uint16_t crc16;
	struct osdp_packet_header *pkt;
	uint8_t *data;
	int data_len;

	/* Do a sanity check only; we expect header to be pre-filled */
	if ((unsigned long)len <= sizeof(struct osdp_packet_header)) {
		LOG_ERR("PKT_F: Invalid header");
		return OSDP_ERR_PKT_FMT;
	}

	if (packet_has_mark(pd)) {
		if (buf[0] != OSDP_PKT_MARK) {
			LOG_ERR("PKT_F: MARK validation failed! ID: 0x%02x",
				is_cp_mode(pd) ? pd->cmd_id : pd->reply_id);
			return OSDP_ERR_PKT_FMT;
		}
		/* temporarily get rid of mark byte */
		buf += 1;
		len -= 1;
		max_len -= 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->som != OSDP_PKT_SOM) {
		LOG_ERR("PKT_F: header SOM validation failed! ID: 0x%02x",
			is_cp_mode(pd) ? pd->cmd_id : pd->reply_id);
		return OSDP_ERR_PKT_FMT;
	}

	/* len: with 2 byte CRC */
	pkt->len_lsb = BYTE_0(len + 2);
	pkt->len_msb = BYTE_1(len + 2);

	if (sc_is_active(pd) &&
	    pkt->control & PKT_CONTROL_SCB && pkt->data[1] >= SCS_15) {
		if (pkt->data[1] == SCS_17 || pkt->data[1] == SCS_18) {
			/**
			 * Only the data portion of message (after id byte)
			 * is encrypted. While (en/de)crypting, we must skip
			 * header, security block, and cmd/reply ID byte.
			 *
			 * Note: if cmd/reply has no data, we must set type to
			 * SCS_15/SCS_16 and send them.
			 */
			data = pkt->data + pkt->data[0] + 1;
			data_len = len - (sizeof(struct osdp_packet_header) +
					  pkt->data[0] + 1);
			len -= data_len;
			/**
			 * check if the passed buffer can hold the encrypted
			 * data where length may be rounded up to the nearest
			 * 16 byte block boundary.
			 */
			if (AES_PAD_LEN(data_len + 1) > max_len) {
				/* data_len + 1 for OSDP_SC_EOM_MARKER */
				goto out_of_space_error;
			}
			len += osdp_encrypt_data(pd, is_cp_mode(pd), data, data_len);
		}
		/* len: with 4bytes MAC; with 2 byte CRC; without 1 byte mark */
		if (len + 4 > max_len) {
			goto out_of_space_error;
		}

		/* len: with 2 byte CRC; with 4 byte MAC */
		pkt->len_lsb = BYTE_0(len + 2 + 4);
		pkt->len_msb = BYTE_1(len + 2 + 4);

		/* compute and extend the buf with 4 MAC bytes */
		osdp_compute_mac(pd, is_cp_mode(pd), buf, len);
		data = is_cp_mode(pd) ? pd->sc.c_mac : pd->sc.r_mac;
		memcpy(buf + len, data, 4);
		len += 4;
	}

	/* fill crc16 */
	if (len + 2 > max_len) {
		goto out_of_space_error;
	}
	crc16 = osdp_compute_crc16(buf, len);
	buf[len + 0] = BYTE_0(crc16);
	buf[len + 1] = BYTE_1(crc16);
	len += 2;

	return len + packet_has_mark(pd);

out_of_space_error:
	LOG_ERR("PKT_F: Out of buffer space! CMD(%02x)", pd->cmd_id);
	return OSDP_ERR_PKT_FMT;
}

int osdp_phy_send_packet(struct osdp_pd *pd, uint8_t *buf,
			 int len, int max_len)
{
	int ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, buf, len, max_len);
	if (len < 0) {
		return OSDP_ERR_PKT_BUILD;
	}

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump(buf, len,
				  "P_TRACE_SEND: %sPD[%d]%s:",
				  is_cp_mode(pd) ? "CP->" : "",
				  pd->address,
				  is_pd_mode(pd) ? "->CP" : "");
		}
	}

	ret = osdp_channel_send(pd, buf, len);
	if (ret != len) {
		LOG_ERR("Channel send for %d bytes failed! ret: %d",
			len, ret);
		return OSDP_ERR_PKT_BUILD;
	}

	return OSDP_ERR_PKT_NONE;
}

static int phy_check_header(struct osdp_pd *pd)
{
	int pkt_len, len, target_len;
	struct osdp_packet_header *pkt;
	uint8_t cur_byte = 0, prev_byte = 0;
	uint8_t *buf = pd->packet_buf;

	while (pd->packet_buf_len == 0) {
		if (osdp_rb_pop(&pd->rx_rb, &cur_byte)) {
			return OSDP_ERR_PKT_NO_DATA;
		}
		if (cur_byte == OSDP_PKT_SOM) {
			if (prev_byte == OSDP_PKT_MARK) {
				buf[0] = OSDP_PKT_MARK;
				buf[1] = OSDP_PKT_SOM;
				pd->packet_buf_len = 2;
				SET_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
			} else {
				buf[0] = OSDP_PKT_SOM;
				pd->packet_buf_len = 1;
				CLEAR_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
			}
			break;
		}
		prev_byte = cur_byte;
	}

	target_len = sizeof(struct osdp_packet_header);
	len = osdp_rb_pop_buf(&pd->rx_rb, buf + pd->packet_buf_len,
			      target_len - pd->packet_buf_len);
	pd->packet_buf_len += len;
	if (pd->packet_buf_len < target_len) {
		return OSDP_ERR_PKT_WAIT;
	}

	pkt = (struct osdp_packet_header *)(buf + packet_has_mark(pd));

	/* validate packet header */
	if (pkt->som != OSDP_PKT_SOM) {
		LOG_ERR("Invalid SOM 0x%02x", pkt->som);
		return OSDP_ERR_PKT_FMT;
	}

	if ((is_cp_mode(pd) && !(pkt->pd_address & 0x80)) ||
	    (is_pd_mode(pd) &&  (pkt->pd_address & 0x80))) {
		LOG_WRN("Ignoring packet with invalid PD_ADDR.MSB");
		return OSDP_ERR_PKT_SKIP;
	}

	/* validate packet length */
	pkt_len = (pkt->len_msb << 8) | pkt->len_lsb;

	if (pkt_len > OSDP_PACKET_BUF_SIZE ||
	    (unsigned long)pkt_len < sizeof(struct osdp_packet_header) + 1) {
		return OSDP_ERR_PKT_FMT;
	}

	return pkt_len + packet_has_mark(pd);
}

static int phy_check_packet(struct osdp_pd *pd, uint8_t *buf, int pkt_len)
{
	int pd_addr;
	uint16_t comp, cur;
	struct osdp_packet_header *pkt;

	if (packet_has_mark(pd)) {
		buf += 1;
		pkt_len -= 1;
	}
	pkt = (struct osdp_packet_header *)buf;

	/* validate CRC/checksum */
	if (pkt->control & PKT_CONTROL_CRC) {
		pkt_len -= 2; /* consume CRC */
		cur = (buf[pkt_len + 1] << 8) | buf[pkt_len];
		comp = osdp_compute_crc16(buf, pkt_len);
		if (comp != cur) {
			LOG_ERR("Invalid crc 0x%04x/0x%04x", comp, cur);
			return OSDP_ERR_PKT_FMT;
		}
	} else {
		pkt_len -= 1; /* consume checksum */
		cur = buf[pkt_len];
		comp = osdp_compute_checksum(buf, pkt_len);
		if (comp != cur) {
			LOG_ERR("Invalid checksum %02x/%02x", comp, cur);
			return OSDP_ERR_PKT_FMT;
		}
	}

	/* validate PD address */
	pd_addr = pkt->pd_address & 0x7F;
	if (pd_addr != pd->address && pd_addr != 0x7F) {
		/* not addressed to us and was not broadcasted */
		if (is_cp_mode(pd)) {
			LOG_ERR("Invalid pd address %d", pd_addr);
			return OSDP_ERR_PKT_CHECK;
		}
		return OSDP_ERR_PKT_SKIP;
	}

	/* validate sequence number */
	comp = pkt->control & PKT_CONTROL_SQN;
	if (is_pd_mode(pd)) {
		if (comp == 0) {
			/**
			 * CP is trying to restart communication by sending a 0.
			 * The current PD implementation does not hold any state
			 * between commands so we can just set seq_number to -1
			 * (so it gets incremented to 0 with a call to
			 * phy_get_seq_number()) and invalidate any established
			 * secure channels.
			 */
			pd->seq_number = -1;
			sc_deactivate(pd);
		}
		else if (comp == pd->seq_number) {
			/**
			 * Sometimes, a CP re-sends the same command without
			 * incrementing the sequence number. To handle such cases,
			 * we will move the sequence back one step (with do_inc
			 * set to -1) and then process the packet all over again
			 * as if it was the first time we are seeing it.
			 */
			pd->seq_number -= 1;
			LOG_INF("received a sequence repeat packet!");
		}
		/**
		 * For packets addressed to the broadcast address, the reply
		 * must have address set to 0x7F and not the current PD's
		 * address. To handle this case, let's capture this state in PD
		 * flags.
		 */
		if (pd_addr == 0x7F) {
			SET_FLAG(pd, PD_FLAG_PKT_BROADCAST);
		}
	} else {
		if (comp == 0) {
			/**
			 * Check for receiving a busy reply from the PD which would
			 * have a sequence number of 0, come in an unsecured packet
			 * of minimum length, and have the reply ID REPLY_BUSY.
			 */
			if ((pkt_len == 6) && (pkt->data[0] == REPLY_BUSY)) {
				pd->seq_number -= 1;
				return OSDP_ERR_PKT_BUSY;
			}
		}
	}
	cur = osdp_phy_get_seq_number(pd, is_pd_mode(pd));
	if (cur != comp && !ISSET_FLAG(pd, PD_FLAG_SKIP_SEQ_CHECK)) {
		LOG_ERR("packet seq mismatch %d/%d", cur, comp);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_SEQ_NUM;
		return OSDP_ERR_PKT_NACK;
	}

	return OSDP_ERR_PKT_NONE;
}

int osdp_phy_check_packet(struct osdp_pd *pd)
{
	int ret = OSDP_ERR_PKT_FMT;

	ret = osdp_channel_receive(pd); /* always pull new bytes first */

	/**
	 * PD mode does not maintain state. When we receive the anything
	 * from CP, we need to capture the timestamp so we can timeout and
	 * clear the buffer on errors and stray RX data.
	 */
	if (is_pd_mode(pd) && pd->packet_buf_len == 0 && ret > 0) {
		pd->tstamp = osdp_millis_now();
	}

	if (pd->packet_len == 0) {
		ret = phy_check_header(pd);
		if (ret < 0) {
			return ret;
		}
		pd->packet_len = ret;
	}

	/* We have a valid header, collect one full packet */
	ret = osdp_rb_pop_buf(&pd->rx_rb, pd->packet_buf + pd->packet_buf_len,
				pd->packet_len - pd->packet_buf_len);
	pd->packet_buf_len += ret;
	if (pd->packet_buf_len != pd->packet_len)
		return OSDP_ERR_PKT_WAIT;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		/**
		 * A crude way of identifying and NOT printing poll messages
		 * when CONFIG_OSDP_PACKET_TRACE is enabled.
		 *
		 * OSDP_CMD_ID_OFFSET + 2 is also checked as the CMD_ID can be
		 * pushed back by 2 bytes if secure channel block is present in
		 * header.
		 */
		ret = OSDP_CMD_ID_OFFSET + packet_has_mark(pd);
		if (sc_is_active(pd)) {
			ret += 2;
		}
		if ((is_cp_mode(pd) && pd->cmd_id != CMD_POLL) ||
		    (is_pd_mode(pd) && pd->packet_buf_len > ret &&
		     pd->packet_buf[ret] != CMD_POLL)) {
			osdp_dump(pd->packet_buf, pd->packet_buf_len,
				  "P_TRACE_RECV: %sPD[%d]%s:",
				  is_pd_mode(pd) ? "CP->" : "",
				  pd->address,
				  is_cp_mode(pd) ? "->CP" : "");
		}
	}

	return phy_check_packet(pd, pd->packet_buf, pd->packet_len);
}

int osdp_phy_decode_packet(struct osdp_pd *pd, uint8_t **pkt_start)
{
	uint8_t *data, *mac, *buf = pd->packet_buf;
	int mac_offset, is_cmd, len = pd->packet_buf_len;
	struct osdp_packet_header *pkt;

	if (packet_has_mark(pd)) {
		buf += 1;
		len -= 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	len -= pkt->control & PKT_CONTROL_CRC ? 2 : 1;
	mac_offset = len - 4;
	data = pkt->data;
	len -= sizeof(struct osdp_packet_header);

	if (pkt->control & PKT_CONTROL_SCB) {
		if (is_pd_mode(pd) && !sc_is_capable(pd)) {
			LOG_ERR("PD is not SC capable");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_UNSUP;
			return OSDP_ERR_PKT_NACK;
		}
		if (pkt->data[1] < SCS_11 || pkt->data[1] > SCS_18) {
			LOG_ERR("Invalid SB Type");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_NACK;
		}
		if (!sc_is_active(pd) && pkt->data[1] > SCS_14) {
			LOG_ERR("Invalid SCS type (%x)", pkt->data[1]);
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_NACK;
		}
		if (pkt->data[1] == SCS_11 || pkt->data[1] == SCS_13) {
			/**
			 * CP signals PD to use SCBKD by setting SCB data byte
			 * to 0. But since SCBK-D is insecure access, it's
			 * usage is limited to install mode (a provisioning time
			 * mode) only.
			 */
			if (ISSET_FLAG(pd, OSDP_FLAG_INSTALL_MODE) &&
			    pkt->data[2] == 0) {
				SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			}
		}
		data = pkt->data + pkt->data[0];
		len -= pkt->data[0]; /* consume security block */
	} else {
		/**
		 * If the current packet is an ACK for a KEYSET, the PD might
		 * have discarded the secure channel session keys in favour of
		 * the new key we sent and hence this packet may reach us in
		 * plain text. To work with such PDs, we must also discard our
		 * secure session.
		 *
		 * The way we do this is by calling osdp_keyset_complete() which
		 * copies the key in ephemeral_data to the current SCBK.
		 */
		if (is_cp_mode(pd) && pd->cmd_id == CMD_KEYSET &&
		    pkt->data[0] == REPLY_ACK) {
			osdp_keyset_complete(pd);
		}

		if (sc_is_active(pd)) {
			LOG_ERR("Received plain-text message in SC");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_NACK;
		}
	}

	if (sc_is_active(pd) &&
	    pkt->control & PKT_CONTROL_SCB && pkt->data[1] >= SCS_15) {
		/* validate MAC */
		is_cmd = is_pd_mode(pd);
		osdp_compute_mac(pd, is_cmd, buf, mac_offset);
		mac = is_cmd ? pd->sc.c_mac : pd->sc.r_mac;
		if (memcmp(buf + mac_offset, mac, 4) != 0) {
			LOG_ERR("Invalid MAC; discarding SC");
			sc_deactivate(pd);
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_NACK;
		}
		len -= 4; /* consume MAC */

		/* decrypt data block */
		if (pkt->data[1] == SCS_17 || pkt->data[1] == SCS_18) {
			/**
			 * Only the data portion of message (after id byte)
			 * is encrypted. While (en/de)crypting, we must skip
			 * header (6), security block (2) and cmd/reply id (1)
			 * bytes if cmd/reply has no data, use SCS_15/SCS_16.
			 *
			 * At this point, the header and security block is
			 * already consumed. So we can just skip the cmd/reply
			 * ID (data[0])  when calling osdp_decrypt_data().
			 */
			len = osdp_decrypt_data(pd, is_cmd, data + 1, len - 1);
			if (len < 0) {
				LOG_ERR("Failed at decrypt; discarding SC");
				sc_deactivate(pd);
				pd->reply_id = REPLY_NAK;
				pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
				return OSDP_ERR_PKT_NACK;
			}
			if (len == 0) {
				/**
				 * If cmd/reply has no data, PD "should" have
				 * used SCS_15/SCS_16 but we will be tolerant
				 * towards those faulty implementations.
				 */
				LOG_INF("Received encrypted data block with 0 "
					"length; tolerating non-conformance!");
			}
			len += 1; /* put back cmd/reply ID */
		}
	}

	*pkt_start = data;
	return len;
}

void osdp_phy_state_reset(struct osdp_pd *pd, bool is_error)
{
	pd->packet_buf_len = 0;
	pd->packet_len = 0;
	pd->phy_state = 0;
	if (is_error) {
		pd->seq_number = -1;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
	}
}

#ifdef UNIT_TESTING
int (*test_osdp_phy_packet_finalize)(struct osdp_pd *pd, uint8_t *buf,
			int len, int max_len) = osdp_phy_packet_finalize;
#endif
