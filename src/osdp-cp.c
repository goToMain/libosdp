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
    if (to_cp(ctx) != NULL)
        free(to_cp(ctx));
    if (to_pd(ctx, 0) != NULL)
        free(to_pd(ctx, 0));
}

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info)
{
    int i;
    osdp_t *ctx;

    ctx = calloc(1, sizeof(osdp_t));
    if (ctx == NULL) {
        print(ctx, LOG_ERR, "Failed to alloc osdp_t");
        return NULL;
    }

    ctx->cp = calloc(1, sizeof(cp_t));
    if (ctx->cp == NULL) {
        print(ctx, LOG_ERR, "Failed to alloc cp_t");
        free(ctx);
        return NULL;
    }

    ctx->pd = calloc(1, sizeof(pd_t) * num_pd);
    if (ctx->pd == NULL) {
        print(ctx, LOG_ERR, "Failed to alloc cp_t[]");
        free(ctx->cp);
        free(ctx);
        return NULL;
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

    print(ctx, LOG_INFO, "cp setup complete");

    return (osdp_cp_t *)ctx;
}

void osdp_cp_refresh(osdp_cp_t *ctx)
{
    int n, i;

    n = to_cp(ctx)->num_pd;

    for (i=0; i<n; i++) {
        cp_fsm_execute(to_osdp(ctx));
    }
}
