/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 23:00:44 IST 2019
 */

#include <string.h>
#include "cp-private.h"

#define PKT_CONTROL_SQN             0x03
#define PKT_CONTROL_CRC             0x04

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

int cp_build_packet_head(pd_t *p, uint8_t *buf, int maxlen)
{
    int exp_len;
    struct osdp_packet_header *pkt;

    exp_len = sizeof(struct osdp_packet_header);
    if (maxlen < exp_len) {
        osdp_log(LOG_NOTICE, "pkt_buf len err - %d/%d", maxlen, exp_len);
        return -1;
    }

    /* Fill packet header */
    pkt = (struct osdp_packet_header *)buf;
    pkt->mark = 0xFF;
    pkt->som = 0x53;
    pkt->pd_address = p->address & 0xFF;  /* Use only the lower 8 bits */
    pkt->control = cp_get_seq_number(p, TRUE);
    pkt->control |= PKT_CONTROL_CRC;

    return sizeof(struct osdp_packet_header);
}

int cp_build_packet_tail(pd_t *p, uint8_t *buf, int len, int maxlen)
{
    uint16_t crc16;
    struct osdp_packet_header *pkt;

    /* expect head to be prefilled */
    if (buf[0] != 0xFF || buf[1] != 0x53)
        return -1;

    pkt = (struct osdp_packet_header *)buf;

    /* fill packet length into header w/ 2b crc; wo/ 1b mark */
    pkt->len_lsb = byte_0(len - 1 + 2);
    pkt->len_msb = byte_1(len - 1 + 2);

    /* fill crc16 */
    crc16 = compute_crc16(buf + 1, len - 1); /* excluding mark byte */
    buf[len + 0] = byte_0(crc16);
    buf[len + 1] = byte_1(crc16);
    len += 2;

    return len;
}

int cp_decode_packet(pd_t *p, uint8_t *buf, int blen)
{
    int pkt_len;
    uint16_t comp, cur;
    struct osdp_packet_header *pkt;

    /* validate packet header */
    pkt = (struct osdp_packet_header *)buf;
    if (pkt->mark != 0xFF) {
        osdp_log(LOG_ERR, "invalid marking byte '0x%x'", pkt->mark);
        return -1;
    }
    if (pkt->som != 0x53) {
        osdp_log(LOG_ERR, "invalid mark SOM '%d'", pkt->som);
        return -1;
    }
    if (pkt->pd_address != (p->address & 0xFF)) {
        osdp_log(LOG_ERR, "invalid pd address %d", pkt->pd_address);
        return -1;
    }
    pkt_len = (pkt->len_msb << 8) | pkt->len_lsb;
    if (pkt_len != blen - 1) {
        osdp_log(LOG_ERR, "packet length mismatch %d/%d", pkt_len, blen - 1);
        return -1;
    }
    cur = pkt->control & PKT_CONTROL_SQN;
    comp = cp_get_seq_number(p, FALSE);
    if (comp != cur && !isset_flag(p, PD_FLAG_SKIP_SEQ_CHECK)) {
        osdp_log(LOG_ERR, "packet seq mismatch %d/%d", comp, cur);
        return -1;
    }
    blen -= sizeof(struct osdp_packet_header); /* consume header */

    /* validate CRC/checksum */
    if (pkt->control & PKT_CONTROL_CRC) {
        cur = (buf[pkt_len] << 8) | buf[pkt_len - 1];
        blen -= 2; /* consume 2byte CRC */
        comp = compute_crc16(buf + 1, pkt_len - 2);
        if (comp != cur) {
            osdp_log(LOG_ERR, "invalid crc 0x%04x/0x%04x", comp, cur);
            return -1;
        }
    } else {
        cur = buf[blen - 1];
        blen -= 1; /* consume 1byte checksum */
        comp = compute_checksum(buf + 1, pkt_len - 1);
        if (comp != cur) {
            osdp_log(LOG_ERR, "invalid checksum 0x%02x/0x%02x", comp, cur);
            return -1;
        }
    }

    /* copy decoded message block into cmd buf */
    memcpy(buf, pkt->data, blen);
    return blen;
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int cp_build_command(pd_t *p, struct cmd *cmd, uint8_t *buf, int maxlen)
{
    union cmd_all *c;
    int ret=-1, i, len = 0;

    switch(cmd->id) {
    case CMD_POLL:
    case CMD_LSTAT:
    case CMD_ISTAT:
    case CMD_OSTAT:
    case CMD_RSTAT:
        buf[len++] = cmd->id;
        ret = 0;
        break;
    case CMD_ID:
    case CMD_CAP:
    case CMD_DIAG:
        buf[len++] = cmd->id;
        buf[len++] = 0x00;
        ret = 0;
        break;
    case CMD_OUT:
        if (cmd->len != sizeof(struct cmd) + 4)
            break;
        c = (union cmd_all *)cmd->data;
        buf[len++] = cmd->id;
        buf[len++] = c->output.output_no;
        buf[len++] = c->output.control_code;
        buf[len++] = byte_0(c->output.tmr_count);
        buf[len++] = byte_1(c->output.tmr_count);
        ret = 0;
        break;
    case CMD_LED:
        if (cmd->len != sizeof(struct cmd) + 16)
            break;
        c = (union cmd_all *)cmd->data;
        buf[len++] = cmd->id;
        buf[len++] = c->led.reader;
        buf[len++] = c->led.number;

        buf[len++] = c->led.temperory.control_code;
        buf[len++] = c->led.temperory.on_count;
        buf[len++] = c->led.temperory.off_count;
        buf[len++] = c->led.temperory.on_color;
        buf[len++] = c->led.temperory.off_color;
        buf[len++] = byte_0(c->led.temperory.timer);
        buf[len++] = byte_1(c->led.temperory.timer);

        buf[len++] = c->led.permanent.control_code;
        buf[len++] = c->led.permanent.on_count;
        buf[len++] = c->led.permanent.off_count;
        buf[len++] = c->led.permanent.on_color;
        buf[len++] = c->led.permanent.off_color;
        ret = 0;
        break;
    case CMD_BUZ:
        if (cmd->len != sizeof(struct cmd) + 5)
            break;
        c = (union cmd_all *)cmd->data;
        buf[len++] = cmd->id;
        buf[len++] = c->buzzer.reader;
        buf[len++] = c->buzzer.tone_code;
        buf[len++] = c->buzzer.on_count;
        buf[len++] = c->buzzer.off_count;
        buf[len++] = c->buzzer.rep_count;
        ret = 0;
        break;
    case CMD_TEXT:
        if (cmd->len != sizeof(struct cmd) + 38)
            break;
        c = (union cmd_all *)cmd->data;
        buf[len++] = cmd->id;
        buf[len++] = c->text.reader;
        buf[len++] = c->text.cmd;
        buf[len++] = c->text.temp_time;
        buf[len++] = c->text.offset_row;
        buf[len++] = c->text.offset_col;
        buf[len++] = c->text.length;
        for (i=0; i<c->text.length; i++)
            buf[len++] = c->text.data[i];
        ret = 0;
        break;
    case CMD_COMSET:
        if (cmd->len != sizeof(struct cmd) + 5)
            break;
        c = (union cmd_all *)cmd->data;
        buf[len++] = cmd->id;
        buf[len++] = c->comset.addr;
        buf[len++] = byte_0(c->comset.baud);
        buf[len++] = byte_1(c->comset.baud);
        buf[len++] = byte_2(c->comset.baud);
        buf[len++] = byte_3(c->comset.baud);
        ret = 0;
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
        osdp_log(LOG_ERR, "command 0x%02x isn't supported", cmd->id);
        break;
    case CMD_SCDONE:
    case CMD_XWR:
    case CMD_SPE:
    case CMD_CONT:
    case CMD_RMODE:
    case CMD_XMIT:
        osdp_log(LOG_ERR, "command 0x%02x is obsolete", cmd->id);
        break;
    default:
        osdp_log(LOG_ERR, "command 0x%02x is unrecognized", cmd->id);
        break;
    }

    if (ret < 0) {
        osdp_log(LOG_WARNING, "cmd 0x%02x format error! -- %d", cmd->id, ret);
        return ret;
    }

    return len;
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
 *  0: success
 * -1: error
 *  2: retry current command
 */
int cp_decode_response(pd_t *p, uint8_t *buf, int len)
{
    osdp_t *ctx = to_ctx(p);
    int i, ret=-1, reply_id, pos=0;
    uint32_t temp32;

    reply_id = buf[pos++];
    len--; /* consume reply id from the head */

    osdp_log(LOG_DEBUG, "Processing resp 0x%02x with %d data bytes", reply_id, len);

    switch(reply_id) {
    case REPLY_ACK:
        ret = 0;
        break;
    case REPLY_NAK:
        if (buf[pos]) {
            osdp_log(LOG_ERR, get_nac_reason(buf[pos]));
        }
        ret = 0;
        break;
    case REPLY_PDID:
        if (len != 12) {
            osdp_log(LOG_DEBUG, "PDID format error, %d/%d", len, pos);
            break;
        }
        p->id.vendor_code  = buf[pos++];
        p->id.vendor_code |= buf[pos++] << 8;
        p->id.vendor_code |= buf[pos++] << 16;

        p->id.model = buf[pos++];
        p->id.version = buf[pos++];

        p->id.serial_number  = buf[pos++];
        p->id.serial_number |= buf[pos++] << 8;
        p->id.serial_number |= buf[pos++] << 16;
        p->id.serial_number |= buf[pos++] << 24;

        p->id.firmware_version  = buf[pos++] << 16;
        p->id.firmware_version |= buf[pos++] << 8;
        p->id.firmware_version |= buf[pos++];
        ret = 0;
        break;
    case REPLY_PDCAP:
        if (len % 3 != 0) {
            osdp_log(LOG_DEBUG, "PDCAP format error, %d/%d", len, pos);
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
        osdp_log(LOG_CRIT, "COMSET responded with ID:%d baud:%d", i, temp32);
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
            ctx->cp->cardread_handler(p->address, fmt, buf + pos, key_len);
        }
        ret = 0;
        break;
    }
    case REPLY_BUSY:
        /* PD busy; signal upper layer to retry command */
        ret = 2;
        break;
    case REPLY_ISTATR:
    case REPLY_OSTATR:
    case REPLY_BIOREADR:
    case REPLY_BIOMATCHR:
    case REPLY_MFGREP:
    case REPLY_XRD:
        osdp_log(LOG_ERR, "unsupported reply: 0x%02x", reply_id);
        ret = 0;
        break;
    case REPLY_SCREP:
    case REPLY_PRES:
    case REPLY_SPER:
        osdp_log(LOG_ERR, "deprecated reply: 0x%02x", reply_id);
        ret = 0;
        break;
    default:
        osdp_log(LOG_DEBUG, "unexpected reply: 0x%02x", reply_id);
        break;
    }
    return ret;
}

int cp_send_command(pd_t *p, struct cmd *cmd)
{
    int ret, len;
    uint8_t buf[512];

    if ((len = cp_build_packet_head(p, buf, 512)) < 0) {
        osdp_log(LOG_ERR, "failed to build_packet_head");
        return -1;
    }

    if ((ret = cp_build_command(p, cmd, buf + len, 512 - len)) < 0) {
        osdp_log(LOG_ERR, "failed to build command %d", cmd->id);
        return -1;
    }
    len += ret;

    if ((len = cp_build_packet_tail(p, buf, len, 512)) < 0) {
        osdp_log(LOG_ERR, "failed to build command %d", cmd->id);
        return -1;
    }

    ret = p->send_func(buf, len);

    return (ret == len) ? 0 : -1;
}

/**
 * Returns:
 *  0: success
 * -1: error
 *  1: no data yet
 *  2: re-issue command
 */
int cp_process_response(pd_t *p)
{
    int len;
    uint8_t resp[512];

    len = p->recv_func(resp, 512);
    if (len <= 0)
        return 1; // no data

    if ((len = cp_decode_packet(p, resp, len)) < 0) {
        osdp_log(LOG_ERR, "failed decode response");
        return -1;
    }
    return cp_decode_response(p, resp, len);
}

int cp_enqueue_command(pd_t *p, struct cmd *c)
{
    int len, fs, start, end;
    struct cmd_queue *q = p->queue;

    fs = q->tail - q->head;
    if (fs <= 0)
        fs += OSDP_PD_CMD_QUEUE_SIZE;

    len = c->len;
    if (len > fs)
        return -1;

    start = q->head + 1;
    if (start >= OSDP_PD_CMD_QUEUE_SIZE)
        start = 0;

    if (start == q->tail)
        return -1;

    end = start + len;
    if (end >= OSDP_PD_CMD_QUEUE_SIZE)
        end = end % OSDP_PD_CMD_QUEUE_SIZE;

    if (start > end) {
        memcpy(q->buffer + start, c, OSDP_PD_CMD_QUEUE_SIZE - start);
        memcpy(q->buffer, (uint8_t *)c + OSDP_PD_CMD_QUEUE_SIZE - start, end);
    } else {
        memcpy(q->buffer + start, c, len);
    }

    q->head = end;
    return 0;
}

int cp_dequeue_command(pd_t *pd, int readonly, uint8_t *cmd_buf, int maxlen)
{
    int start, end, len;
    struct cmd_queue *q = pd->queue;

    if (q->head == q->tail)
        return 0; /* empty */

    start = q->tail + 1;
    len = q->buffer[start];

    if (len > maxlen)
        return -1;

    end = start + len;
    if(end >= OSDP_PD_CMD_QUEUE_SIZE)
        end = end % OSDP_PD_CMD_QUEUE_SIZE;

    if (start > end) {
        memcpy(cmd_buf, q->buffer + start, OSDP_PD_CMD_QUEUE_SIZE - start);
        memcpy(cmd_buf + OSDP_PD_CMD_QUEUE_SIZE - start, q->buffer, end);
    } else {
        memcpy(cmd_buf, q->buffer + start, len);
    }

    if (!readonly)
        q->tail = end;
    return len;
}

enum {
    CP_PHY_STATE_IDLE,
    CP_PHY_STATE_SEND_CMD,
    CP_PHY_STATE_RESP_WAIT,
    CP_PHY_STATE_ERR,
};

/**
 * Returns:
 *  -1: phy is in error state. Main state machine must reset it.
 *   0: No command in queue
 *   1: Command is in progress; this method must to be called again later.
 *   2: In command boundaries. There may or may not be other commands in queue.
 *
 * Note: This method must not dequeue cmd unless it reaches an invalid state.
 */
int cp_phy_state_update(pd_t *pd)
{
    int ret=0;
    struct cmd *cmd = (struct cmd *)pd->scratch;

    switch(pd->phy_state) {
    case CP_PHY_STATE_IDLE:
        ret = cp_dequeue_command(pd, FALSE, pd->scratch, OSDP_PD_SCRATCH_SIZE);
        if (ret == 0)
            break;
        if (ret < 0) {
            osdp_log(LOG_INFO, "command dequeue error");
            pd->phy_state = CP_PHY_STATE_ERR;
            break;
        }
        ret = 1;
        /* no break */
    case CP_PHY_STATE_SEND_CMD:
        if ((cp_send_command(pd, cmd)) < 0) {
            osdp_log(LOG_INFO, "command dispatch error");
            pd->phy_state = CP_PHY_STATE_ERR;
            ret = -1;
            break;
        }
        pd->phy_state = CP_PHY_STATE_RESP_WAIT;
        pd->phy_tstamp = millis_now();
        break;
    case CP_PHY_STATE_RESP_WAIT:
        if ((ret = cp_process_response(pd)) == 0) {
            pd->phy_state = CP_PHY_STATE_IDLE;
            ret = 2;
            break;
        }
        if (ret == -1) {
            pd->phy_state = CP_PHY_STATE_ERR;
            break;
        }
        if (ret == 2) {
            osdp_log(LOG_INFO, "PD busy; retry last command");
            pd->phy_state = CP_PHY_STATE_SEND_CMD;
            break;
        }
        if (millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
            osdp_log(LOG_INFO, "read response timeout");
            pd->phy_state = CP_PHY_STATE_ERR;
            ret = 1;
        }
        break;
    case CP_PHY_STATE_ERR:
        ret = -1;
        break;
    }

    return ret;
}

void cp_phy_state_reset(pd_t *pd)
{
    pd->state = 0;
    pd->seq_number = -1;
}
