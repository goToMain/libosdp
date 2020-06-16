/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "osdp_common.h"

#define TAG "PHY: "

#define PKT_CONTROL_SQN			0x03
#define PKT_CONTROL_CRC			0x04
#define PKT_CONTROL_SCB			0x08

struct osdp_packet_header {
	uint8_t mark;
	uint8_t som;
	uint8_t pd_address;
	uint8_t len_lsb;
	uint8_t len_msb;
	uint8_t control;
	uint8_t data[];
};

uint8_t compute_checksum(uint8_t *msg, int length)
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

const char *get_nac_reason(int code)
{
	const char *osdp_nak_reasons[] = {
		"",
		"Message check character(s) error (bad cksum/crc)",
		"Command length error",
		"Unknown Command Code. Command not implemented by PD",
		"Unexpected sequence number detected in the header",
		"This PD does not support the security block that was received",
		"Communication security conditions not met",
		"BIO_TYPE not supported",
		"BIO_FORMAT not supported",
		"Unable to process command record",
	};

	if (code < 0 || code >= OSDP_PD_NAK_SENTINEL)
		code = 0;

	return osdp_nak_reasons[code];
}

static int phy_get_seq_number(struct osdp_pd *p, int do_inc)
{
	/* p->seq_num is set to -1 to reset phy cmd state */
	if (do_inc) {
		p->seq_number += 1;
		if (p->seq_number > 3)
			p->seq_number = 1;
	}
	return p->seq_number & PKT_CONTROL_SQN;
}

uint8_t *phy_packet_get_data(struct osdp_pd *p, const uint8_t *buf)
{
	int sb_len = 0;
	struct osdp_packet_header *pkt;

	ARG_UNUSED(p);
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->control & PKT_CONTROL_SCB)
		sb_len = pkt->data[0];

	return pkt->data + sb_len;
}

uint8_t *phy_packet_get_smb(struct osdp_pd *p, const uint8_t *buf)
{
	struct osdp_packet_header *pkt;

	ARG_UNUSED(p);
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->control & PKT_CONTROL_SCB)
		return pkt->data;

	return NULL;
}

int phy_in_sc_handshake(int is_reply, int id)
{
	if (is_reply)
		return (id == REPLY_CCRYPT || id == REPLY_RMAC_I);
	else
		return (id == CMD_CHLNG || id == CMD_SCRYPT);
}

int phy_build_packet_head(struct osdp_pd *p, int id, uint8_t *buf, int maxlen)
{
	int exp_len, pd_mode, sb_len;
	struct osdp_packet_header *pkt;

	pd_mode = isset_flag(p, PD_FLAG_PD_MODE);
	exp_len = sizeof(struct osdp_packet_header) + 64; /* estimated cmd len */
	if (maxlen < exp_len) {
		LOG_N(TAG "pkt_buf len err - %d/%d", maxlen,
			 exp_len);
		return -1;
	}

	/* Fill packet header */
	pkt = (struct osdp_packet_header *)buf;
	pkt->mark = 0xFF;
	pkt->som = 0x53;
	pkt->pd_address = p->address & 0x7F;	/* Use only the lower 7 bits */
	if (pd_mode)
		pkt->pd_address |= 0x80;
	pkt->control = phy_get_seq_number(p, !isset_flag(p, PD_FLAG_PD_MODE));
	pkt->control |= PKT_CONTROL_CRC;

	sb_len = 0;
	if (isset_flag(p, PD_FLAG_SC_ACTIVE)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = sb_len = 2;
		pkt->data[1] = SCS_15;
	} else if (phy_in_sc_handshake(pd_mode, id)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = sb_len = 3;
		pkt->data[1] = SCS_11;
	}

	return sizeof(struct osdp_packet_header) + sb_len;
}

int phy_build_packet_tail(struct osdp_pd *p, uint8_t *buf, int len, int maxlen)
{
	int i, is_cmd, data_len;
	uint8_t *data;
	uint16_t crc16;
	struct osdp_packet_header *pkt;

	/* expect head to be prefilled */
	if (buf[0] != 0xFF || buf[1] != 0x53)
		return -1;

	is_cmd = !isset_flag(p, PD_FLAG_PD_MODE);
	pkt = (struct osdp_packet_header *)buf;

	if (isset_flag(p, PD_FLAG_SC_ACTIVE) &&
	    pkt->control & PKT_CONTROL_SCB &&
	    pkt->data[1] >= SCS_15)
	{
		if (pkt->data[1] == SCS_17 || pkt->data[1] == SCS_18) {
			/**
			 * Only the data portion of message (after id byte)
			 * is encrypted. While (en/de)crypting, we must skip
			 * header, security block, and cmd/reply ID
			 * bytes if cmd/reply has no data, use SCS_15/SCS_16.
			 */
			data = pkt->data + pkt->data[0] + 1;
			data_len = len - sizeof(struct osdp_packet_header)
						- pkt->data[0] - 1;
			len -= data_len;
			/**
			 * check if the passed buffer can hold the encrypted
			 * data where length may be rounded up to the nearest
			 * 16 byte block bondary.
			 */
			if (AES_PAD_LEN(data_len + 1) > maxlen)
				return -1;
			len += osdp_encrypt_data(p, is_cmd, data, data_len);
		}
		/* len: with 4bytes MAC; with 2 byte CRC; without 1 byte mark */
		if (len + 4 > maxlen)
			return -1;
		pkt->len_lsb = byte_0(len - 1 + 2 + 4);
		pkt->len_msb = byte_1(len - 1 + 2 + 4);

		/* compute and extend the buf with 4 MAC bytes */
		osdp_compute_mac(p, is_cmd, buf + 1, len - 1);
		data = is_cmd ? p->sc.c_mac : p->sc.r_mac;
		for (i = 0; i < 4; i++)
			buf[len + i] = data[i];
		len += 4;
	} else {
		/* len: with 2 byte CRC; without 1 byte mark */
		pkt->len_lsb = byte_0(len - 1 + 2);
		pkt->len_msb = byte_1(len - 1 + 2);
	}
	/* fill crc16 */
	if (len + 2 > maxlen)
		return -1;
	crc16 = compute_crc16(buf + 1, len - 1);	/* excluding mark byte */
	buf[len + 0] = byte_0(crc16);
	buf[len + 1] = byte_1(crc16);
	len += 2;

	return len;
}

/**
 * Returns:
 *  +ve: length of decoded packet.
 *   -1: fatal errors
 *   -2: incomplete data
 *   -3: soft fail. upper layers must skip this message
 *   -4: packet format errors
 */
int phy_decode_packet(struct osdp_pd *p, uint8_t *buf, int len)
{
	uint8_t *data, *mac;
	uint16_t comp, cur;
	int is_cmd, pkt_len, pd_mode, pd_addr, mac_offset;
	struct osdp_packet_header *pkt;

	pd_mode = isset_flag(p, PD_FLAG_PD_MODE);
	pkt = (struct osdp_packet_header *)buf;

	/* validate packet header */
	if (!pd_mode && !(pkt->pd_address & 0x80)) {
		LOG_E(TAG "reply without MSB set 0x%02x", pkt->pd_address);
		return -1;
	}

	/* validate packet length */
	pkt_len = (pkt->len_msb << 8) | pkt->len_lsb;
	if (pkt_len != len - 1) {
		LOG_E(TAG "packet length mismatch %d/%d", pkt_len, len - 1);
		return -2;
	}

	/* validate PD address */
	pd_addr = pkt->pd_address & 0x7F;
	if (pd_addr != p->address) {
		if (!pd_mode) {
			LOG_E(TAG "invalid pd address %d", pd_addr);
			return -1;
		} else {
			LOG_D(TAG "cmd for PD[%d] discarded", pd_addr);
			return -3;
		}
	}

	/* validate sequence number */
	cur = pkt->control & PKT_CONTROL_SQN;
	comp = phy_get_seq_number(p, pd_mode);
	if (comp != cur && !isset_flag(p, PD_FLAG_SKIP_SEQ_CHECK)) {
		LOG_E(TAG "packet seq mismatch %d/%d", comp, cur);
		return -1;
	}
	len -= sizeof(struct osdp_packet_header);	/* consume header */

	/* validate CRC/checksum */
	if (pkt->control & PKT_CONTROL_CRC) {
		cur = (buf[pkt_len] << 8) | buf[pkt_len - 1];
		comp = compute_crc16(buf + 1, pkt_len - 2);
		if (comp != cur) {
			LOG_E(TAG "invalid crc 0x%04x/0x%04x", comp, cur);
			return -1;
		}
		mac_offset = pkt_len - 4 - 2;
		len -= 2; /* consume CRC */
	} else {
		comp = compute_checksum(buf + 1, pkt_len - 1);
		if (comp != buf[len - 1]) {
			LOG_E(TAG "invalid checksum 0x%02x/0x%02x", comp, cur);
			return -1;
		}
		mac_offset = pkt_len - 4 - 1;
		len -= 1; /* consume checksum */
	}

	data = pkt->data;

	if (pkt->control & PKT_CONTROL_SCB) {
		if (pkt->data[1] < SCS_11 || pkt->data[1] > SCS_18) {
			LOG_E(TAG "invalid SB Type");
			return -1;
		}
		if (pkt->data[1] == SCS_11 || pkt->data[1] == SCS_13) {
			/**
			 * CP signals PD to use SCBKD by setting SB data byte
			 * to 0. In CP, PD_FLAG_SC_USE_SCBKD comes from FSM; on
			 * PD we extract it from the command itself. But this
			 * usage of SCBKD is allowed only when the PD is in
			 * install mode (indicated by PD_FLAG_INSTALL_MODE).
			 */
			if (isset_flag(p, PD_FLAG_INSTALL_MODE) &&
			    pkt->data[2] == 0)
				set_flag(p, PD_FLAG_SC_USE_SCBKD);
		}
		data = pkt->data + pkt->data[0];
		len -= pkt->data[0]; /* consume security block */
	} else {
		if (isset_flag(p, PD_FLAG_SC_ACTIVE)) {
			LOG_E(TAG "got plain text in SC");
			return -1;
		}
	}

	if (isset_flag(p, PD_FLAG_SC_ACTIVE) &&
		pkt->control & PKT_CONTROL_SCB &&
		pkt->data[1] >= SCS_15)
	{
		/* validate MAC */
		is_cmd = isset_flag(p, PD_FLAG_PD_MODE);
		osdp_compute_mac(p, is_cmd, buf + 1, mac_offset);
		mac = is_cmd ? p->sc.c_mac : p->sc.r_mac;
		if (memcmp(buf + 1 + mac_offset, mac, 4) != 0) {
			LOG_E(TAG "invalid MAC");
			return -1;
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
			 * already consumed.
			 */
			len -= 1; /* remove ID from len before decrypting */
			len = osdp_decrypt_data(p, is_cmd, data + 1, len);
			if (len <= 0) {
				LOG_E(TAG "failed at decrypt");
				return -1;
			}
			len += 1; /* put back ID */
		}
	}

	memmove(buf, data, len);
	return len;
}

void phy_state_reset(struct osdp_pd *pd)
{
	pd->phy_state = 0;
	pd->seq_number = -1;
}
