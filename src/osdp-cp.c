/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#include <stdlib.h>

#include "cp-private.h"

void osdp_cp_teardown(osdp_cp_t *ctx)
{
    cp_t *cp;
    pd_t *pd;

    if (ctx == NULL)
        return;

    /* teardown pd */
    pd = to_pd(ctx, 0);
    if (pd != NULL) {
        free(pd);
    }

    /* teardown cp */
    cp = to_cp(ctx);
    if (cp != NULL) {
        safe_free(cp->queue);
        free(cp);
    }

    free(ctx);
}

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info)
{
    int i;
    osdp_t *ctx;

    ctx = calloc(1, sizeof(osdp_t));
    if (ctx == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc osdp_t");
        goto malloc_err;
    }

    ctx->cp = calloc(1, sizeof(cp_t));
    if (ctx->cp == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc cp_t");
        goto malloc_err;
    }

    ctx->cp->queue = calloc(1, sizeof(struct cmd_queue) * num_pd);
    if (ctx->cp->queue == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc cp->cmd_queue");
        goto malloc_err;
    }

    ctx->pd = calloc(1, sizeof(pd_t) * num_pd);
    if (ctx->pd == NULL) {
        osdp_log(LOG_ERR, "Failed to alloc cp_t[]");
        goto malloc_err;
    }

    ctx->magic = 0xDEADBEAF;
    ctx->log_level = LOG_WARNING;
    to_cp(ctx)->num_pd = num_pd;
    for (i=0; i<num_pd; i++) {
        osdp_pd_info_t *p = info + i;
        to_pd(ctx, i)->baud_rate = p->baud_rate;
        to_pd(ctx, i)->address = p->address;
        to_pd(ctx, i)->flags = p->init_flags;
        to_pd(ctx, i)->seq_number = -1;
    }

    osdp_log(LOG_INFO, "cp setup complete");

    return (osdp_cp_t *)ctx;

malloc_err:
    osdp_cp_teardown((osdp_cp_t *)ctx);
    return NULL;
}

void osdp_cp_refresh(osdp_cp_t *ctx)
{
    int n, i;

    n = to_cp(ctx)->num_pd;

    for (i=0; i<n; i++) {
        cp_fsm_execute(to_osdp(ctx));
    }
}

int osdp_set_output(osdp_cp_t *ctx, int pd, int output_no, int control_code,
                    int timer)
{
    return 0;
}

int osdp_set_led(osdp_cp_t *ctx, int pd, int on_color, int off_color,
                 int on_count, int off_count, int rep_count)
{
    return 0;
}
int osdp_set_buzzer(osdp_cp_t *ctx, int pd, int on_count, int off_count,
                    int rep_count)
{
    return 0;
}

int osdp_set_text(osdp_cp_t *ctx, int pd, int duration, int row, int col,
                  const char *msg)
{
    return 0;
}

int osdp_set_params(osdp_cp_t *ctx, int pd, int pd_address, uint32_t baud_rate)
{
    return 0;
}
