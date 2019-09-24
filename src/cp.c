/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#include <stdlib.h>
#include <string.h>

#include "cp-private.h"

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info)
{
    int i;
    pd_t *pd;
    cp_t *cp;
    osdp_t *ctx;

    ctx = calloc(1, sizeof(osdp_t));
    if (ctx == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc osdp_t");
        goto malloc_err;
    }
    ctx->magic = 0xDEADBEAF;

    ctx->cp = calloc(1, sizeof(cp_t));
    if (ctx->cp == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc cp_t");
        goto malloc_err;
    }
    cp = to_cp(ctx);
    child_set_parent(cp, ctx);
    cp->num_pd = num_pd;

    ctx->pd = calloc(1, sizeof(pd_t) * num_pd);
    if (ctx->pd == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc cp_t[]");
        goto malloc_err;
    }

    for (i=0; i<num_pd; i++) {
        osdp_pd_info_t *p = info + i;
        pd = to_pd(ctx, i);
        ctx->pd->queue = calloc(1, sizeof(struct cmd_queue));
        if (ctx->pd->queue == NULL) {
            osdp_log(LOG_ERR, "Failed to alloc pd->cmd_queue");
            goto malloc_err;
        }
        child_set_parent(pd, ctx);
        pd->baud_rate = p->baud_rate;
        pd->address = p->address;
        pd->flags = p->init_flags;
        pd->seq_number = -1;
        pd->send_func = p->send_func;
        pd->recv_func = p->recv_func;
    }

    osdp_set_log_level(LOG_WARNING);
    osdp_log(LOG_INFO, "cp setup complete");
    return (osdp_cp_t *)ctx;

malloc_err:
    osdp_cp_teardown((osdp_cp_t *)ctx);
    return NULL;
}

void osdp_cp_teardown(osdp_cp_t *ctx)
{
    int i;
    cp_t *cp;
    pd_t *pd;

    if (ctx == NULL)
        return;

    cp = to_cp(ctx);
    if (cp == NULL)
        return;

    for (i = cp->num_pd - 1; i >= 0; i--) {
        pd = to_pd(ctx, i);
        if (pd != NULL) {
            if (pd->queue != NULL)
                free(pd->queue);
            if (i == 0)
                free(pd); // final
        }
    }

    free(cp);
    free(ctx);
}

void osdp_cp_refresh(osdp_cp_t *ctx)
{
    int i;

    for (i = 0; i < to_cp(ctx)->num_pd; i++) {
        set_current_pd(ctx, i);
        cp_state_update(to_current_pd(ctx));
    }
}

int osdp_set_output(osdp_cp_t *ctx, int pd, int op_no, int ctrl_code, int timer)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;
    struct cmd_output *data = (struct cmd_output *)cmd->data;
    pd_t *p = to_pd(ctx, pd);

    cmd->id = CMD_OUT;
    cmd->len = sizeof(struct cmd) + sizeof(struct cmd_output);
    data->control_code = ctrl_code;
    data->output_no = op_no;
    data->tmr_count = timer;

    if (cp_enqueue_command(p, cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_OUT enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_set_led(osdp_cp_t *ctx, int pd, int led, int on_color, int off_color,
                 int on_count, int off_count, int rep_count)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;
    struct cmd_led *data = (struct cmd_led *)cmd->data;
    pd_t *p = to_pd(ctx, pd);

    cmd->id = CMD_OUT;
    cmd->len = sizeof(struct cmd) + sizeof(struct cmd_led);

    data->reader = 0;
    data->number = led;
    data->temperory.control_code = 0x00;
    data->permanent.control_code = 0x00;
    if (rep_count) {
        data->temperory.control_code = 0x02;
        data->temperory.on_color = on_color;
        data->temperory.off_color = off_color;
        data->temperory.on_count = on_count;
        data->temperory.off_count = off_count;
        data->temperory.timer = (uint16_t)((on_count + off_count) * rep_count);
    } else {
        data->permanent.control_code = 0x01;
        data->permanent.on_color = on_color;
        data->permanent.off_color = off_color;
        data->permanent.on_count = on_count;
        data->permanent.off_count = off_count;
    }
    if (cp_enqueue_command(p, cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_OUT enqueue error!");
        return -1;
    }
    return 0;
}
int osdp_set_buzzer(osdp_cp_t *ctx, int pd, int on_count, int off_count, int rep_count)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;
    struct cmd_buzzer *data = (struct cmd_buzzer *)cmd->data;
    pd_t *p = to_pd(ctx, pd);

    cmd->id = CMD_BUZ;
    cmd->len = sizeof(struct cmd) + sizeof(struct cmd_buzzer);

    data->reader = 0;
    data->tone_code = 0;
    data->on_count = on_count;
    data->off_count = off_count;
    data->rep_count = rep_count;

    if (cp_enqueue_command(p, cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_set_text(osdp_cp_t *ctx, int pd, int cmd_code, int duration, int row,
                  int col, const char *msg)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;
    struct cmd_text *data = (struct cmd_text *)cmd->data;
    pd_t *p = to_pd(ctx, pd);

    int len = strlen(msg);
    if (len > 32) {
        osdp_log(LOG_WARNING, "CMD_TEXT length of msg too long!");
    }

    cmd->id = CMD_TEXT;
    cmd->len = sizeof(struct cmd) + sizeof(struct cmd_text);

    data->reader = 0;
    data->cmd = cmd_code;
    data->temp_time = duration;
    data->offset_row = row;
    data->offset_col = col;
    data->length = len;
    memcpy(data->data, msg, len); // non-null terminated

    if (cp_enqueue_command(p, cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_set_params(osdp_cp_t *ctx, int pd, int pd_address, uint32_t baud_rate)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;
    struct cmd_comset *data = (struct cmd_comset *)cmd->data;
    pd_t *p = to_pd(ctx, pd);

    cmd->id = CMD_COMSET;
    cmd->len = sizeof(struct cmd) + sizeof(struct cmd_comset);

    data->addr = pd_address;
    data->baud = baud_rate;

    if (cp_enqueue_command(p, cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}
