/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 23:00:44 IST 2019
 */

#include "cp-private.h"

struct osdp_packet_header {
    uint8_t mark;
    uint8_t som;
    uint8_t pd_address;
    uint8_t len_lsb;
    uint8_t len_msb;
    uint8_t control;
    uint8_t data[0];
};

#define OSDP_PKT_CONTROL_SQN 0x03
#define OSDP_PKT_CONTROL_CRC 0x04

static int cp_get_seq_number(pd_t *p, int do_inc)
{
    /* p->seq_num is set to -1 to reset phy cmd state */
    if (do_inc) {
        p->seq_number += 1;
        if (p->seq_number > 3)
            p->seq_number = 1;
    }
    return p->seq_number & OSDP_PKT_CONTROL_SQN;
}

int cp_build_packet(osdp_t *ctx, uint8_t *buf, int blen, uint8_t cmd, int clen)
{
    int pkt_len, crc16;
    struct osdp_packet_header *pkt;
    pd_t *p = to_current_pd(ctx);

    /* Fill packet header */
    pkt = (struct osdp_packet_header *)buf;
    pkt->mark = 0xFF;
    pkt->som = 0x53;
    pkt->pd_address = p->address & 0xFF;  /* Use only the lower 8 bits */
    pkt->control = get_seq_number(p, TRUE);
    pkt->control |= OSDP_PKT_CONTROL_CRC;
    pkt_len = sizeof(struct osdp_packet_header);

    p->seq_number += 1;
    memcpy(pkt->data, cmd, clen);
    pkt_len += clen;

    /* fill packet length into header with the  2 bytes for crc16 */
    pkt->len_lsb = BYTE_0(pkt_len + 2);
    pkt->len_msb = BYTE_1(pkt_len + 2);

    /* fill crc16 */
    crc16 = compute_crc16(buf, pkt_len);
    buf[pkt_len + 0] = BYTE_0(crc16);
    buf[pkt_len + 1] = BYTE_1(crc16);
    pkt_len += 2;

    return pkt_len;
}

#define return_if(c) if ((c)) return -1;

int cp_decode_packet(osdp_t *ctx, uint8_t *buf, int blen, uint8_t *cmd, int clen)
{
    int pkt_len, comp, cur;
    struct osdp_packet_header *pkt;
    pd_t *p = to_current_pd(ctx);

    /* validate packet header */
    pkt = (struct osdp_packet_header *)buf;
    if (pkt->mark != 0xFF) {
        print(ctx, LOG_ERR, "invalid marking byte '0x%x'", pkt->mark);
        return -1;
    }
    if (pkt->som != 0x53) {
        print(ctx, LOG_ERR, "invalid mark SOM '%d'", pkt->som);
        return -1;
    }
    if (pkt->pd_address != (p->address & 0xFF)) {
        print(ctx, LOG_ERR, "invalid pd address %d", pkt->pd_address);
        return -1;
    }
    pkt_len = (pkt->len_msb << 8) | pkt->len_lsb;
    if (pkt_len != blen) {
        print(ctx, LOG_ERR, "packet length mismatch %d/%d", pkt_len, clen);
        return -1;
    }
    blen -= sizeof(struct osdp_packet_header);

    /* validate CRC/checksum */
    if (pkt->control & OSDP_PKT_CONTROL_CRC) {
        comp = compute_crc16(buf, pkt_len - 2);
        cur = (buf[pkt_len - 1] << 8) | buf[pkt_len - 2];
        if (comp != cur) {
            print(ctx, LOG_ERR, "invalid crc 0x%04x/0x%04x", comp, cur);
            return -1;
        }
        blen -= 2;
    } else {
        comp = compute_checksum(buf, pkt_len - 1);
        cur = buf[pkt_len-1];
        if (comp != cur) {
            print(ctx, LOG_ERR, "invalid checksum 0x%02x/0x%02x", comp, cur);
            return -1;
        }
        blen -= 1;
    }

    /* copy decoded message block into cmd buf */
    if (blen > clen) {
        print(ctx, LOG_ERR, "Not enough space cmd buf");
        return -1;
    }
    memcpy(cmd, pkt->data, blen);

    return blen;
}
