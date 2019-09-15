/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 23:00:44 IST 2019
 */

#include "cp-private.h"

#define PKT_CONTROL_SQN 0x03
#define PKT_CONTROL_CRC 0x04

struct osdp_packet_header {
    uint8_t mark;
    uint8_t som;
    uint8_t pd_address;
    uint8_t len_lsb;
    uint8_t len_msb;
    uint8_t control;
    uint8_t data[0];
};

static int cp_get_seq_number(pd_t *p, int do_inc)
{
    /* p->seq_num is set to -1 to reset phy cmd state */
    if (do_inc) {
        p->seq_number += 1;
        if (p->seq_number > 3)
            p->seq_number = 1;
    }
    return p->seq_number & PKT_CONTROL_SQN;
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
    pkt->control |= PKT_CONTROL_CRC;
    pkt_len = sizeof(struct osdp_packet_header);

    p->seq_number += 1;
    memcpy(pkt->data, cmd, clen);
    pkt_len += clen;

    /* fill packet length into header with the  2 bytes for crc16 */
    pkt->len_lsb = byte_0(pkt_len + 2);
    pkt->len_msb = byte_1(pkt_len + 2);

    /* fill crc16 */
    crc16 = compute_crc16(buf, pkt_len);
    buf[pkt_len + 0] = byte_0(crc16);
    buf[pkt_len + 1] = byte_1(crc16);
    pkt_len += 2;

    return pkt_len;
}

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
    if (pkt->control & PKT_CONTROL_CRC) {
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

/**
 * Returns:
 *  0: command built successfully
 * -1: error
 */
int cp_build_command(osdp_t *ctx, struct cmd *cmd, uint8_t *buf, int blen)
{
    union cmd_all *c;
    int ret=-1, i, len = 0;

    buf[len++] = cmd->id;
    switch(cmd->id) {
    case CMD_POLL:
    case CMD_LSTAT:
    case CMD_ISTAT:
    case CMD_OSTAT:
    case CMD_RSTAT:
        ret = 0;
        break;
    case CMD_ID:
        buf[len++] = 0x00;
        ret = 0;
        break;
    case CMD_CAP:
        buf[len++] = 0x00;
        ret = 0;
        break;
    case CMD_DIAG:
        buf[len++] = 0x00;
        print(ctx, LOG_DEBUG, "cmd:0x%02x is not yet defined", cmd->id);
        break;
    case CMD_OUT:
        if (cmd->arg == NULL)
            return 0;
        c = cmd->arg;
        buf[len++] = c->output->output_no;
        buf[len++] = c->output->control_code;
        buf[len++] = byte_0(c->output->tmr_count);
        buf[len++] = byte_1(c->output->tmr_count);
        break;
    case CMD_LED:
        if (cmd->arg == NULL)
            return 0;
        c = cmd->arg;
        buf[len++] = c->led->reader;
        buf[len++] = c->led->number;

        buf[len++] = c->led->temperory.control_code;
        buf[len++] = c->led->temperory.on_time;
        buf[len++] = c->led->temperory.off_time;
        buf[len++] = c->led->temperory.on_color;
        buf[len++] = c->led->temperory.off_color;
        buf[len++] = byte_0(c->led->temperory.timer);
        buf[len++] = byte_1(c->led->temperory.timer);

        buf[len++] = c->led->temperory.control_code;
        buf[len++] = c->led->temperory.on_time;
        buf[len++] = c->led->temperory.off_time;
        buf[len++] = c->led->temperory.on_color;
        buf[len++] = c->led->temperory.off_color;
        break;
    case CMD_BUZ:
        if (cmd->arg == NULL)
            return 0;
        c = cmd->arg;
        buf[len++] = c->buzzer->reader;
        buf[len++] = c->buzzer->tone_code;
        buf[len++] = c->buzzer->on_time;
        buf[len++] = c->buzzer->off_time;
        buf[len++] = c->buzzer->rep_count;
        break;
    case CMD_TEXT:
        if (cmd->arg == NULL)
            return 0;
        c->text = cmd->arg;
        buf[len++] = c->text->reader;
        buf[len++] = c->text->cmd;
        buf[len++] = c->text->temp_time;
        buf[len++] = c->text->offset_row;
        buf[len++] = c->text->offset_col;
        buf[len++] = c->text->length;
        for (i=0; i<c->text->length; i++)
            buf[len++] = c->text->data[i];
        break;
    case CMD_COMSET:
        if (cmd->arg == NULL)
            return 0;
        c = cmd->arg;
        buf[len++] = c->comset->addr;
        buf[len++] = byte_0(c->comset->baud);
        buf[len++] = byte_1(c->comset->baud);
        buf[len++] = byte_2(c->comset->baud);
        buf[len++] = byte_3(c->comset->baud);
        break;
    case CMD_KEYSET:
    case CMD_CHLNG:
    case CMD_SCRYPT:
    case CMD_PROMPT:
    case CMD_BIOREAD:
    case CMD_BIOMATCH:
    case CMD_TDSET:
    case CMD_DATA:
    case CMD_ABORT:
    case CMD_MAXREPLY:
    case CMD_MFG:
        print(ctx, LOG_ERR, "command 0x%02x isn't supported", cmd->id);
        return 0;
    case CMD_SCDONE:
    case CMD_XWR:
    case CMD_SPE:
    case CMD_CONT:
    case CMD_RMODE:
    case CMD_XMIT:
        print(ctx, LOG_ERR, "command 0x%02x is obsolete", cmd->id);
        return 0;
    default:
        print(ctx, LOG_ERR, "command 0x%02x is unrecognized", cmd->id);
        return 0;
    }
}

const char *get_nac_reason(int code)
{
    const char *nak_reasons[] = {
        "",
        "NAK: Message check character(s) error (bad cksum/crc)",
        "NAK: Command length error",
        "NAK: Unknown Command Code. Command not implemented by PD",
        "NAK: Unexpected sequence number detected in the header",
        "NAK: This PD does not support the security block that was received",
        "NAK: Communication security conditions not met",
        "NAK: BIO_TYPE not supported",
        "NAK: BIO_FORMAT not supported",
        "NAK: Unable to process command record",
    };
    if (code < 0 || code >= sizeof_array(nak_reasons))
        code = 0;
    return nak_reasons[code];
}

/**
 * Returns:
 *  1: retry current command
 *  0: got a valid response
 * -1: error
 */
int cp_process_response(osdp_t *ctx, uint8_t *buf, int len)
{
    int i, ret=-1, reply_id, pos=0;
    uint32_t temp32;
    pd_t *p = to_current_pd(ctx);

    reply_id = buf[pos++];

    switch(reply_id) {
    case REPLY_ACK:
        ret = 0;
        break;
    case REPLY_NAK:
        if (buf[pos]) {
            print(ctx, LOG_ERR, get_nac_reason(buf[pos]));
        }
        ret = 0;
        break;
    case REPLY_PDID:
    {
        struct pd_id *s = &p->id;

        if (len != 12) {
            print(ctx, LOG_DEBUG, "PDID format error, %d/%d", len, pos);
            break;
        }

        s->vendor_code  = buf[pos++];
        s->vendor_code |= buf[pos++] << 8;
        s->vendor_code |= buf[pos++] << 16;

        s->model = buf[pos++];
        s->version = buf[pos++];

        s->serial_number  = buf[pos++];
        s->serial_number |= buf[pos++] << 8;
        s->serial_number |= buf[pos++] << 16;
        s->serial_number |= buf[pos++] << 24;

        s->firmware_version  = buf[pos++] << 16;
        s->firmware_version |= buf[pos++] << 8;
        s->firmware_version |= buf[pos++];
        ret = 0;
        break;
    }
    case REPLY_PDCAP:
        if (len % 3 != 0) {
            print(ctx, LOG_DEBUG, "PDCAP format error, %d/%d", len, pos);
            break;
        }
        while (pos < len) {
            int cap_code = buf[pos++];
            p->cap[cap_code].compliance_level = buf[pos++];
            p->cap[cap_code].num_items = buf[pos++];
        }
        ret = 0;
        break;
    case REPLY_LSTATR:
        buf[pos++] ? set_flag(p, PD_FLAG_TAMPER) : clear_flag(p, PD_FLAG_TAMPER);
        buf[pos++] ? set_flag(p, PD_FLAG_POWER) : clear_flag(p, PD_FLAG_POWER);
        ret = 0;
        break;
    case REPLY_RSTATR:
        buf[pos++] ? set_flag(p, PD_FLAG_R_TAMPER) : clear_flag(p, PD_FLAG_R_TAMPER);
        ret = 0;
        break;
    case REPLY_COM:
        i = buf[pos++];
        temp32  = buf[pos++];
        temp32 |= buf[pos++] << 8;
        temp32 |= buf[pos++] << 16;
        temp32 |= buf[pos++] << 24;
        print(ctx, LOG_CRIT, "COMSET responded with ID:%d baud:%d", i, temp32);
        p->baud_rate = temp32;
        set_flag(p, PD_FLAG_COMSET_INPROG);
        ret = 0;
        break;
    case REPLY_CCRYPT:
    case REPLY_RMAC_I:
    case REPLY_KEYPPAD:
    {
        char key;
        int klen;

        pos++; /* skip one byte */
        klen = buf[pos++];

        if (ctx->cp->keypress_handler) {
            for (i=0; i<klen; i++) {
                key = buf[pos + i];
                ctx->cp->keypress_handler(p->address, key);
            }
        }
        ret = 0;
        break;
    }
    case REPLY_RAW:
    {
        int fmt, len;

        pos++; /* skip one byte */
        fmt = buf[pos++];
        len  = buf[pos++];
        len |= buf[pos++] << 8;
        if (ctx->cp->cardread_handler) {
            ctx->cp->cardread_handler(p->address, fmt, buf + pos, len);
        }
        ret = 0;
        break;
    }
    case REPLY_FMT:
    {
        int key_len, fmt;

        pos++; /* skip one byte */
        pos++; /* skip one byte -- TODO: discuss about reader direction */
        key_len = buf[pos++];
        fmt = OSDP_CARD_FMT_ASCII;
        if (ctx->cp->cardread_handler) {
            ctx->cp->cardread_handler(p->address, fmt, key_len, buf + pos);
        }
        ret = 0;
        break;
    }
    case REPLY_BUSY:
        /* PD busy; signal upper layer to retry command */
        ret = 1;
        break;
    case REPLY_ISTATR:
    case REPLY_OSTATR:
    case REPLY_BIOREADR:
    case REPLY_BIOMATCHR:
    case REPLY_MFGREP:
    case REPLY_XRD:
        print(ctx, LOG_ERR, "unsupported reply: 0x%02x", reply_id);
        ret = 0;
        break;
    case REPLY_SCREP:
    case REPLY_PRES:
    case REPLY_SPER:
        osdp_log(ctx, LOG_ERR, "deprecated reply: 0x%02x", reply_id);
        ret = 0;
        break;
    }
    if (ret == -1) {
        osdp_log(ctx, LOG_DEBUG, "unexpected reply: 0x%02x", reply_id);
    }

    return ret;
}
