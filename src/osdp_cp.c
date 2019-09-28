/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_cp_private.h"

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info)
{
    int i;
    pd_t *pd;
    cp_t *cp;
    osdp_t *ctx;

    if (num_pd <= 0 || info == NULL)
        return NULL;

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
        osdp_log(LOG_ERR, "Failed to alloc pd_t[]");
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
    set_current_pd(ctx, 0);
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

int osdp_cp_set_notifiers(osdp_cp_t *ctx, struct osdp_cp_notifiers *n)
{
    memcpy(&(to_osdp(ctx))->notifier, n, sizeof(struct osdp_cp_notifiers));
    return 0;
}

int osdp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;

    cmd->id = CMD_OUT;
    cmd->len = sizeof(struct cmd) + sizeof(struct osdp_cmd_output);
    memcpy(cmd->data, p, sizeof(struct osdp_cmd_output));

    if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_OUT enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;

    cmd->id = CMD_OUT;
    cmd->len = sizeof(struct cmd) + sizeof(struct osdp_cmd_led);
    memcpy(cmd->data, p, sizeof(struct osdp_cmd_led));

    if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_OUT enqueue error!");
        return -1;
    }
    return 0;
}
int osdp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;

    cmd->id = CMD_BUZ;
    cmd->len = sizeof(struct cmd) + sizeof(struct osdp_cmd_buzzer);
    memcpy(cmd->data, p, sizeof(struct osdp_cmd_buzzer));

    if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_set_text(osdp_cp_t *ctx, int pd, struct osdp_cmd_text *p)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;

    cmd->id = CMD_TEXT;
    cmd->len = sizeof(struct cmd) + sizeof(struct osdp_cmd_text);
    memcpy(cmd->data, p, sizeof(struct osdp_cmd_text));

    if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}

int osdp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p)
{
    uint8_t cmd_buf[64];
    struct cmd *cmd = (struct cmd *)cmd_buf;

    cmd->id = CMD_COMSET;
    cmd->len = sizeof(struct cmd) + sizeof(struct osdp_cmd_comset);
    memcpy(cmd->data, p, sizeof(struct osdp_cmd_comset));

    if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
        osdp_log(LOG_WARNING, "CMD_BUZ enqueue error!");
        return -1;
    }
    return 0;
}
