/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include "osdp_pd_private.h"

osdp_pd_t *osdp_pd_setup(int num_pd, osdp_pd_info_t * p)
{
	int fc;
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;
	struct pd_cap *cap;

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		osdp_log(LOG_ERR, "Failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		osdp_log(LOG_ERR, "Failed to alloc struct osdp_cp");
		goto malloc_err;
	}
	cp = to_cp(ctx);
	child_set_parent(cp, ctx);
	cp->num_pd = 1;

	ctx->pd = calloc(1, sizeof(struct osdp_pd));
	if (ctx->pd == NULL) {
		osdp_log(LOG_ERR, "Failed to alloc struct osdp_pd");
		goto malloc_err;
	}
	set_current_pd(ctx, 0);
	pd = to_pd(ctx, 0);

	pd->cmd_handler = calloc(1, sizeof(struct pd_cmd_handler));
	if (pd->cmd_handler == NULL) {
		osdp_log(LOG_ERR, "Failed to alloc pd_cmd_handler");
		goto malloc_err;
	}

	child_set_parent(pd, ctx);
	pd->baud_rate = p->baud_rate;
	pd->address = p->address;
	pd->flags = p->init_flags;
	pd->seq_number = -1;
	pd->send_func = p->send_func;
	pd->recv_func = p->recv_func;
	memcpy(&pd->id, &p->id, sizeof(struct pd_id));
	cap = pd->cap;
	while (cap && cap->function_code > 0) {
		fc = pd->cap->function_code;
		if (fc >= CAP_SENTINEL)
			break;
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}

	set_flag(pd, PD_FLAG_PD_MODE);	/* used to understand operational mode */

	osdp_set_log_level(LOG_WARNING);
	osdp_log(LOG_INFO, "pd setup complete");
	return (osdp_pd_t *) ctx;

 malloc_err:
	osdp_pd_teardown((osdp_pd_t *) ctx);
	return NULL;
}

void osdp_pd_teardown(osdp_pd_t *ctx)
{
	struct osdp_cp *cp;
	struct osdp_pd *pd;

	if (ctx == NULL)
		return;

	/* teardown PD */
	pd = to_pd(ctx, 0);
	if (pd != NULL) {
		if (pd->cmd_handler != NULL)
			free(pd->cmd_handler);
		free(pd);
	}

	/* teardown CP */
	cp = to_cp(ctx);
	if (cp != NULL) {
		free(cp);
	}

	free(ctx);
}

void osdp_pd_refresh(osdp_pd_t *ctx)
{
	struct osdp_pd *pd = to_current_pd(ctx);

	pd_phy_state_update(pd);
}

int osdp_pd_set_cmd_handlers(osdp_pd_t *ctx, struct pd_cmd_handler *h)
{
	struct osdp_pd *pd = to_current_pd(ctx);

	memcpy(pd->cmd_handler, h, sizeof(struct pd_cmd_handler));
	return 0;
}
