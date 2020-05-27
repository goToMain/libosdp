/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "osdp_common.h"

#define TAG "SC: "

/* Default key as specified in OSDP protocol specification */
static const uint8_t osdp_scbk_default[16] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

void osdp_compute_scbk(struct osdp_pd *p, uint8_t *scbk)
{
	int i;
	struct osdp *ctx = to_ctx(p);

	memcpy(scbk, p->sc.pd_client_uid, 8);
	for (i = 8; i < 16; i++)
		scbk[i] = ~scbk[i - 8];
	osdp_encrypt(ctx->sc_master_key, NULL, scbk, 16);
}

void osdp_compute_session_keys(struct osdp *ctx)
{
	int i;
	struct osdp_pd *p = to_current_pd(ctx);

	if (isset_flag(p, PD_FLAG_SC_USE_SCBKD)) {
		memcpy(p->sc.scbk, osdp_scbk_default, 16);
	} else {
		/**
		 * Compute SCBK only in CP mode. PD mode, expect to already have
		 * the SCBK (sent from application layer).
		 */
		if (isset_flag(p, PD_FLAG_PD_MODE) == 0)
			osdp_compute_scbk(p, p->sc.scbk);
	}

	memset(p->sc.s_enc, 0, 16);
	memset(p->sc.s_mac1, 0, 16);
	memset(p->sc.s_mac2, 0, 16);

	p->sc.s_enc[0]  = 0x01;  p->sc.s_enc[1]  = 0x82;
	p->sc.s_mac1[0] = 0x01;  p->sc.s_mac1[1] = 0x01;
	p->sc.s_mac2[0] = 0x01;  p->sc.s_mac2[1] = 0x02;

	for (i = 2; i < 8; i++) {
		p->sc.s_enc[i]  = p->sc.cp_random[i - 2];
		p->sc.s_mac1[i] = p->sc.cp_random[i - 2];
		p->sc.s_mac2[i] = p->sc.cp_random[i - 2];
	}

	osdp_encrypt(p->sc.scbk, NULL, p->sc.s_enc,  16);
	osdp_encrypt(p->sc.scbk, NULL, p->sc.s_mac1, 16);
	osdp_encrypt(p->sc.scbk, NULL, p->sc.s_mac2, 16);

#if 0
    LOG_D(TAG "Session Keys");
    osdp_dump("SCBK", p->sc.scbk, 16);
    osdp_dump("M-KEY", ctx->sc_master_key, 16);
    osdp_dump("S-ENC", p->sc.s_enc, 16);
    osdp_dump("S-MAC1", p->sc.s_mac1, 16);
    osdp_dump("S-MAC2", p->sc.s_mac2, 16);
#endif
}

void osdp_compute_cp_cryptogram(struct osdp_pd *p)
{
	/* cp_cryptogram = AES-ECB( pd_random[8] || cp_random[8], s_enc ) */
	memcpy(p->sc.cp_cryptogram + 0, p->sc.pd_random, 8);
	memcpy(p->sc.cp_cryptogram + 8, p->sc.cp_random, 8);
	osdp_encrypt(p->sc.s_enc, NULL, p->sc.cp_cryptogram, 16);
}

/**
 * Like memcmp; but operates at constant time.
 *
 * Returns:
 *    0: if memory pointed to by s1 and and s2 are identical.
 *  +ve: if the they are different.
 */
int ct_compare(const void *s1, const void *s2, size_t len)
{
	size_t i, ret = 0;
	const uint8_t *_s1 = s1;
	const uint8_t *_s2 = s2;

	for (i = 0; i < len; i++) {
		if (_s1[i] != _s2[i])
			ret++;
	}

	return ret;
}

int osdp_verify_cp_cryptogram(struct osdp_pd *p)
{
	uint8_t cp_crypto[16];

	/* cp_cryptogram = AES-ECB( pd_random[8] || cp_random[8], s_enc ) */
	memcpy(cp_crypto + 0, p->sc.pd_random, 8);
	memcpy(cp_crypto + 8, p->sc.cp_random, 8);
	osdp_encrypt(p->sc.s_enc, NULL, cp_crypto, 16);

	if (ct_compare(p->sc.cp_cryptogram, cp_crypto, 16) != 0)
		return -1;

	return 0;
}

void osdp_compute_pd_cryptogram(struct osdp_pd *p)
{
	/* pd_cryptogram = AES-ECB( cp_random[8] || pd_random[8], s_enc ) */
	memcpy(p->sc.pd_cryptogram + 0, p->sc.cp_random, 8);
	memcpy(p->sc.pd_cryptogram + 8, p->sc.pd_random, 8);
	osdp_encrypt(p->sc.s_enc, NULL, p->sc.pd_cryptogram, 16);
}

int osdp_verify_pd_cryptogram(struct osdp_pd *p)
{
	uint8_t pd_crypto[16];

	/* pd_cryptogram = AES-ECB( cp_random[8] || pd_random[8], s_enc ) */
	memcpy(pd_crypto + 0, p->sc.cp_random, 8);
	memcpy(pd_crypto + 8, p->sc.pd_random, 8);
	osdp_encrypt(p->sc.s_enc, NULL, pd_crypto, 16);

	if (ct_compare(p->sc.pd_cryptogram, pd_crypto, 16) != 0)
		return -1;

	return 0;
}

void osdp_compute_rmac_i(struct osdp_pd *p)
{
	/* rmac_i = AES-ECB( AES-ECB( cp_cryptogram, s_mac1 ), s_mac2 ) */
	memcpy(p->sc.r_mac, p->sc.cp_cryptogram, 16);
	osdp_encrypt(p->sc.s_mac1, NULL, p->sc.r_mac, 16);
	osdp_encrypt(p->sc.s_mac2, NULL, p->sc.r_mac, 16);
}

int osdp_decrypt_data(struct osdp_pd *p, int is_cmd, uint8_t *data, int length)
{
	int i;
	uint8_t iv[16];

	if (length % 16 != 0) {
		LOG_E(TAG "decrypt_pkt invalid len:%d", length);
		return -1;
	}

	memcpy(iv, is_cmd ? p->sc.r_mac : p->sc.c_mac, 16);
	for (i = 0; i < 16; i++)
		iv[i] = ~iv[i];

	osdp_decrypt(p->sc.s_enc, iv, data, length);

#if 0
    LOG_D(TAG "Decrypt Data -- is_cmd: %d", is_cmd);
    osdp_dump("C-MAC", p->sc.c_mac, 16);
    osdp_dump("R-MAC", p->sc.r_mac, 16);
    osdp_dump("S-MAC1", p->sc.s_mac1, 16);
    osdp_dump("S-MAC2", p->sc.s_mac2, 16);
    osdp_dump("S-ENC", p->sc.s_enc, 16);
    osdp_dump("IV", iv, 16);
    osdp_dump("Decrypt Data", data, length);
#endif

	while (data[length - 1] == 0x00)
		length--;

	if (data[length - 1] != 0x80)
		return -1;

	data[length - 1] = 0;
	return length - 1;
}

int osdp_encrypt_data(struct osdp_pd *p, int is_cmd, uint8_t *data, int length)
{
	int i, pad_len;
	uint8_t iv[16];

	data[length] = 0x80;  /* append EOM marker */
	pad_len = AES_PAD_LEN(length + 1);
	if ((pad_len - length - 1) > 0)
		memset(data + length + 1, 0, pad_len - length - 1);

	memcpy(iv, is_cmd ? p->sc.r_mac : p->sc.c_mac, 16);
	for (i = 0; i < 16; i++)
		iv[i] = ~iv[i];

	osdp_encrypt(p->sc.s_enc, iv, data, pad_len);
#if 0
    LOG_D(TAG "Encrypt Data -- is_cmd: %d", is_cmd);
    osdp_dump("R-MAC", p->sc.r_mac, 16);
    osdp_dump("C-MAC", p->sc.c_mac, 16);
    osdp_dump("S-MAC1", p->sc.s_mac1, 16);
    osdp_dump("S-MAC2", p->sc.s_mac2, 16);
    osdp_dump("S-ENC", p->sc.s_enc, 16);
    osdp_dump("IV", iv, 16);
    osdp_dump("Encrypt Data", data, pad_len);
#endif
	return pad_len;
}

int osdp_compute_mac(struct osdp_pd *p, int is_cmd, const uint8_t *data, int len)
{
	int pad_len;
	uint8_t buf[OSDP_PACKET_BUF_SIZE] = { 0 };
	uint8_t iv[16];

	memcpy(buf, data, len);
	pad_len = (len % 16 == 0) ? len : AES_PAD_LEN(len);
	if (len % 16 != 0)
		buf[len] = 0x80; /* end marker */

	/**
	 * MAC for data blocks B[1] .. B[N] (post padding) is computed as:
	 * IV1 = R_MAC (or) C_MAC  -- depending on is_cmd
	 * IV2 = B[N-1] after -- AES-CBC ( IV1, B[1] to B[N-1], SMAC-1 )
	 * MAC = AES-ECB ( IV2, B[N], SMAC-2 )
	 */

	memcpy(iv, is_cmd ? p->sc.r_mac : p->sc.c_mac, 16);
	if (pad_len > 16) {
		/* N-1 blocks -- encrypted with SMAC-1 */
		osdp_encrypt(p->sc.s_mac1, iv, buf, pad_len - 16);
		/* N-1 th block is the IV the N th block */
		memcpy(iv, buf + pad_len - 32, 16);
	}
	/* N-th Block encrypted with SMAC-2 == MAC */
	osdp_encrypt(p->sc.s_mac2, iv, buf + pad_len - 16, 16);
	memcpy(is_cmd ? p->sc.c_mac : p->sc.r_mac, buf + pad_len - 16, 16);

#if 0
    LOG_D(TAG "MAC Diagnostics is_cmd: %d", is_cmd);
    osdp_dump("S-MAC1", p->sc.s_mac1, 16);
    osdp_dump("S-MAC2", p->sc.s_mac2, 16);
    osdp_dump("IV", iv, 16);
    osdp_dump("R-MAC", p->sc.r_mac, 16);
    osdp_dump("C-MAC", p->sc.c_mac, 16);
#endif

	return 0;
}

void osdp_sc_init(struct osdp_pd *p)
{
	uint8_t key[16];

	if (isset_flag(p, PD_FLAG_PD_MODE))
		memcpy(key, p->sc.scbk, 16);
	memset(&p->sc, 0, sizeof(struct osdp_secure_channel));
	if (isset_flag(p, PD_FLAG_PD_MODE))
		memcpy(p->sc.scbk, key, 16);

	if (isset_flag(p, PD_FLAG_PD_MODE)) {
		p->sc.pd_client_uid[0] = byte_0(p->id.vendor_code);
		p->sc.pd_client_uid[1] = byte_1(p->id.vendor_code);
		p->sc.pd_client_uid[2] = byte_0(p->id.model);
		p->sc.pd_client_uid[3] = byte_1(p->id.version);
		p->sc.pd_client_uid[4] = byte_0(p->id.serial_number);
		p->sc.pd_client_uid[5] = byte_1(p->id.serial_number);
		p->sc.pd_client_uid[6] = byte_2(p->id.serial_number);
		p->sc.pd_client_uid[7] = byte_3(p->id.serial_number);
	}
}
