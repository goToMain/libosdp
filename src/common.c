/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void print(osdp_t *ctx, int log_level, const char *fmt, ...)
{
    va_list args;
    char *buf;
    pd_t *pd = to_current_pd(ctx);
    const char *levels[] = {
        "EMERG", "ALERT", "CRIT ", "ERROR",
        "WARN ", "NOTIC", "INFO ", "DEBUG"
    };

    if (ctx && log_level > ctx->log_level)
        return;

    va_start(args, fmt);
    vasprintf(&buf, fmt, args);
    va_end(args);

    if (pd == NULL)
        printf("OSDP: %s: %s\n", levels[log_level], buf);
    else
        printf("OSDP: %s: PD[%d]: %s\n", levels[log_level], pd->pd_id, buf);

    free(buf);
}
