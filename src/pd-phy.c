/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Tue Sep 24 19:24:49 IST 2019
 */

#include "common.h"

/**
 * Returns:
 *  0: success
 * -1: error
 *  2: retry current command
 */
int pd_decode_command(pd_t *p, struct cmd *reply, uint8_t *buf, int len)
{
    int i, ret=-1, cmd_id, pos=0;
    union cmd_all cmd;
    osdp_t *ctx = to_ctx(p);
    uint32_t temp32;

    cmd_id = buf[pos++];
    len--;

    switch(cmd_id) {
    case CMD_POLL:
        reply->id = REPLY_ACK;
        ret=0;
        break;
    case CMD_LSTAT:
        reply->id = REPLY_LSTATR; ret=0; break;
    case CMD_ISTAT:
        reply->id = REPLY_ISTATR; ret=0; break;
    case CMD_OSTAT:
        reply->id = REPLY_OSTATR; ret=0; break;
    case CMD_RSTAT:
        reply->id = REPLY_RSTATR; ret=0; break;
    case CMD_ID:
        pos++; // Skip reply type info.
        ret = 0;
        reply->id = REPLY_PDID;
        break;
    case CMD_CAP:
        pos++; // Skip reply type info.
        ret = 0;
        reply->id = REPLY_PDCAP;
        break;
    case CMD_OUT:
        if (len != 4)
            break;
        cmd.output.output_no    = buf[pos++];
        cmd.output.control_code = buf[pos++];
        cmd.output.tmr_count    = buf[pos++];
        cmd.output.tmr_count   |= buf[pos++] << 8;
        if (pd_set_output(&cmd.output) != 0)
            break;
        reply->id = REPLY_OSTATR;
        ret = 0;
        break;
    case CMD_LED:
        if (len != 14)
            break;
        cmd.led.reader                 = buf[pos++];
        cmd.led.number                 = buf[pos++];

        cmd.led.temperory.control_code = buf[pos++];
        cmd.led.temperory.on_count     = buf[pos++];
        cmd.led.temperory.off_count    = buf[pos++];
        cmd.led.temperory.on_color     = buf[pos++];
        cmd.led.temperory.off_color    = buf[pos++];
        cmd.led.temperory.timer        = buf[pos++];
        cmd.led.temperory.timer       |= buf[pos++] << 8;

        cmd.led.permanent.control_code = buf[pos++];
        cmd.led.permanent.on_count     = buf[pos++];
        cmd.led.permanent.off_count    = buf[pos++];
        cmd.led.permanent.on_color     = buf[pos++];
        cmd.led.permanent.off_color    = buf[pos++];
        if (pd_set_led(&cmd.led) != 0)
            break;
        reply->id = REPLY_ACK;
        ret = 0;
    case CMD_BUZ:
        if (len != 5)
            break;
        cmd.buzzer.reader    = buf[pos++];
        cmd.buzzer.tone_code = buf[pos++];
        cmd.buzzer.on_count  = buf[pos++];
        cmd.buzzer.off_count = buf[pos++];
        cmd.buzzer.rep_count = buf[pos++];
        if (pd_set_buzzer(&cmd.buzzer) != 0)
            break;
        reply->id = REPLY_ACK;
        ret = 0;
        break;
    case CMD_TEXT:
        if (len < 7)
            break;
        cmd.text.reader     = buf[pos++];
        cmd.text.cmd        = buf[pos++];
        cmd.text.temp_time  = buf[pos++];
        cmd.text.offset_row = buf[pos++];
        cmd.text.offset_col = buf[pos++];
        cmd.text.offset_col = buf[pos++];
        cmd.text.length     = buf[pos++];
        if (cmd.text.length > 32)
            break;
        for (i = 0; i < cmd.text.length; i++)
            cmd.text.data[i] = buf[pos++];
        if (pd_set_text(&cmd.text) != 0)
            break;
        reply->id = REPLY_ACK;
        ret = 0;
        break;
    case CMD_COMSET:
        if (len != 5)
            break;
        cmd.comset.addr  = buf[pos++];
        cmd.comset.baud  = buf[pos++];
        cmd.comset.baud |= buf[pos++] << 8;
        cmd.comset.baud |= buf[pos++] << 16;
        cmd.comset.baud |= buf[pos++] << 24;
        if (pd_set_comm_params(&cmd.comset))
            break;
        reply->id = REPLY_COM;
        ret = 0;
        break;
    default:
        break;
    }

    if (ret != 0) {
        reply->id = REPLY_NAK;
        reply->data[0] = PD_NAK_RECORD;
        return -1;
    }

    return 0;
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
int pd_build_reply(pd_t *p, struct cmd *reply, uint8_t *buf, int maxlen)
{
    int i, ret_val=-1, len=0;

    switch(reply->id) {
    case REPLY_ACK:
        buf[len++] = reply->id;
        break;
    case REPLY_PDID:
        buf[len++] = reply->id;
        buf[len++] = BYTE_1(p->id.vendor_code);
        buf[len++] = BYTE_2(p->id.vendor_code);
        buf[len++] = BYTE_3(p->id.vendor_code);

        buf[len++] = p->id.model;
        buf[len++] = p->id.version;

        buf[len++] = BYTE_1(p->id.serial_number);
        buf[len++] = BYTE_2(p->id.serial_number);
        buf[len++] = BYTE_3(p->id.serial_number);
        buf[len++] = BYTE_4(p->id.serial_number);

        buf[len++] = BYTE_3(p->id.firmware_version);
        buf[len++] = BYTE_2(p->id.firmware_version);
        buf[len++] = BYTE_1(p->id.firmware_version);
        break;
    case REPLY_PDCAP:
        buf[len++] = reply->id;
        for (i=0; i<CAP_SENTINEL; i++) {
            buf[len++] = i;
            buf[len++] = p->cap[i].compliance_level;
            buf[len++] = p->cap[i].num_items;
        }
        break;
    case REPLY_LSTATR:
        buf[len++] = reply->id;
        buf[len++] = isset_flag(p, PD_FLAG_TAMPER);
        buf[len++] = isset_flag(p, PD_FLAG_POWER);
        break;
    case REPLY_RSTATR:
        buf[len++] = reply->id;
        buf[len++] = isset_flag(p, PD_FLAG_R_TAMPER);
        break;
    case REPLY_COM:
        buf[len++] = reply->id;
        buf[len++] = BYTE_1(p->baud_rate);
        buf[len++] = BYTE_2(p->baud_rate);
        buf[len++] = BYTE_3(p->baud_rate);
        buf[len++] = BYTE_4(p->baud_rate);
        break;
    case REPLY_NAK:
    default:
        buf[len++] = reply->id;
        buf[len++] = (reply->len > sizeof(struct cmd)) ?
                            reply->data[0] : PD_NAK_RECORD;
        break;
    }
    return len;
}

int pd_send_reply(pd_t *p, struct cmd *reply)
{
    int ret, len;
    uint8_t buf[512];

    if ((len = phy_build_packet_head(p, buf, 512)) < 0) {
        osdp_log(LOG_ERR, "failed to phy_build_packet_head");
        return -1;
    }

    if ((ret = pd_build_reply(p, reply, buf + len, 512 - len)) < 0) {
        osdp_log(LOG_ERR, "failed to build command %d", reply->id);
        return -1;
    }
    len += ret;

    if ((len = phy_build_packet_tail(p, buf, len, 512)) < 0) {
        osdp_log(LOG_ERR, "failed to build command %d", reply->id);
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
int pd_process_command(pd_t *p, struct cmd *reply)
{
    int len;
    uint8_t resp[512];

    len = p->recv_func(resp, 512);
    if (len <= 0)
        return 1; // no data

    if ((len = phy_decode_packet(p, resp, len)) < 0) {
        osdp_log(LOG_ERR, "failed decode response");
        return -1;
    }
    return pd_decode_command(p, reply, resp, len);
}

enum {
    PD_PHY_STATE_IDLE,
    PD_PHY_STATE_SEND_REPLY,
    PD_PHY_STATE_ERR,
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
int pd_phy_state_update(pd_t *pd)
{
    int ret=0;
    struct cmd *reply = (struct cmd *)pd->scratch;

    switch(pd->phy_state) {
    case PD_PHY_STATE_IDLE:
        ret = pd_process_command(pd, reply);
        if (ret == 1)
            break;
        if (ret < 0) {
            osdp_log(LOG_INFO, "command dequeue error");
            pd->phy_state = PD_PHY_STATE_ERR;
            break;
        }
        ret = 1;
        pd->phy_state = PD_PHY_STATE_SEND_REPLY;
        break;
    case PD_PHY_STATE_SEND_REPLY:
        if ((ret = pd_send_reply(pd, reply)) == 0) {
            pd->phy_state = PD_PHY_STATE_ERR;
            ret = 2;
            break;
        }
        if (ret == -1) {
            pd->phy_state = PD_PHY_STATE_ERR;
            break;
        }
        pd->phy_state = PD_PHY_STATE_IDLE;
        break;
    case PD_PHY_STATE_ERR:
        ret = -1;
        break;
    }

    return ret;
}
