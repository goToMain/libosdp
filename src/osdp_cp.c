/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "osdp_cp_private.h"

#define TAG "CP: "

#define cp_set_offline(p)					\
	do {							\
		p->state = CP_STATE_OFFLINE;			\
		p->tstamp = millis_now();			\
	} while (0)

#define cp_set_state(p, s)					\
	do {							\
		p->state = s;					\
		clear_flag(p, PD_FLAG_AWAIT_RESP);		\
	} while (0)

#define cp_reset_state(p)					\
	do {							\
		p->state = CP_STATE_INIT;			\
		phy_state_reset(p);				\
		p->flags = 0;					\
	} while (0)

int cp_cmd_dispatcher(struct osdp_pd *p, int cmd)
{
	struct osdp_data c;

	if (isset_flag(p, PD_FLAG_AWAIT_RESP)) {
		clear_flag(p, PD_FLAG_AWAIT_RESP);
		return 0;
	}

	c.id = cmd;
	c.len = sizeof(struct osdp_data);
	if (cp_enqueue_command(p, &c) != 0) {
		LOG_W(TAG "command_enqueue error!");
	}
	set_flag(p, PD_FLAG_AWAIT_RESP);
	return 1;
}

int cp_state_update(struct osdp_pd *pd)
{
	int phy_state;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == 1 ||	/* commands are being executed */
	    phy_state == 2)	/* in-between commands */
		return -1 * phy_state;

	/* phy state error -- cleanup */
	if (phy_state < 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successfull */

	switch (pd->state) {
	case CP_STATE_ONLINE:
		if (millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS)
			break;
		if (cp_cmd_dispatcher(pd, CMD_POLL) == 0)
			pd->tstamp = millis_now();
		break;
	case CP_STATE_OFFLINE:
		if (millis_since(pd->tstamp) > OSDP_PD_ERR_RETRY_SEC)
			cp_reset_state(pd);
		break;
	case CP_STATE_INIT:
		cp_set_state(pd, CP_STATE_IDREQ);
		/* no break */
	case CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0)
			break;
		cp_set_state(pd, CP_STATE_CAPDET);
		/* no break */
	case CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0)
			break;
		if (isset_flag(pd, PD_FLAG_SC_CAPABLE)) {
			cp_set_state(pd, CP_STATE_SC_INIT);
			break;
		}
		cp_set_state(pd, CP_STATE_ONLINE);
		break;
	case CP_STATE_SC_INIT:
		osdp_sc_init(pd);
		cp_set_state(pd, CP_STATE_SC_CHLNG);
		/* no break */
	case CP_STATE_SC_CHLNG:
		if (cp_cmd_dispatcher(pd, CMD_CHLNG) != 0)
			break;
		cp_set_state(pd, CP_STATE_SC_CCRYPT);
		/* no break */
	case CP_STATE_SC_CCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0)
			break;
		cp_set_state(pd, CP_STATE_ONLINE);
		break;
	default:
		break;
	}

	return 0;
}

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t * info)
{
	int i;
	struct osdp_pd *pd;
	struct osdp_cp *cp;
	struct osdp *ctx;

	if (num_pd <= 0 || info == NULL)
		return NULL;

	ctx = calloc(1, sizeof(struct osdp));
	if (ctx == NULL) {
		LOG_E(TAG "failed to alloc struct osdp");
		goto malloc_err;
	}
	ctx->magic = 0xDEADBEAF;

	ctx->cp = calloc(1, sizeof(struct osdp_cp));
	if (ctx->cp == NULL) {
		LOG_E(TAG "failed to alloc struct osdp_cp");
		goto malloc_err;
	}
	cp = to_cp(ctx);
	node_set_parent(cp, ctx);
	cp->num_pd = num_pd;

	ctx->pd = calloc(1, sizeof(struct osdp_pd) * num_pd);
	if (ctx->pd == NULL) {
		LOG_E(TAG "failed to alloc struct osdp_pd[]");
		goto malloc_err;
	}

	for (i = 0; i < num_pd; i++) {
		osdp_pd_info_t *p = info + i;
		pd = to_pd(ctx, i);
		ctx->pd->queue = calloc(1, sizeof(struct cmd_queue));
		if (ctx->pd->queue == NULL) {
			LOG_E(TAG "failed to alloc pd->cmd_queue");
			goto malloc_err;
		}
		node_set_parent(pd, ctx);
		pd->baud_rate = p->baud_rate;
		pd->address = p->address;
		pd->flags = p->init_flags;
		pd->seq_number = -1;
		pd->send_func = p->send_func;
		pd->recv_func = p->recv_func;
	}
	set_current_pd(ctx, 0);
	LOG_I(TAG "setup complete");
	return (osdp_cp_t *) ctx;

 malloc_err:
	osdp_cp_teardown((osdp_cp_t *) ctx);
	return NULL;
}

void osdp_cp_teardown(osdp_cp_t *ctx)
{
	int i;
	struct osdp_cp *cp;
	struct osdp_pd *pd;

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
				free(pd);	// final
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

int osdp_cp_set_callback_key_press(osdp_cp_t * ctx,
	int (*cb) (int address, uint8_t key))
{
	(to_osdp(ctx))->notifier.keypress = cb;

	return 0;
}

int osdp_cp_set_callback_card_read(osdp_cp_t * ctx,
	int (*cb) (int address, int format, uint8_t * data, int len))
{
	(to_osdp(ctx))->notifier.cardread = cb;

	return 0;
}

int osdp_cp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_OUT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_output);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_output));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_OUT enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_OUT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_led);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_led));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_OUT enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_BUZ;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_buzzer);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_buzzer));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_set_text(osdp_cp_t *ctx, int pd, struct osdp_cmd_text *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_TEXT;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_text);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_text));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}

int osdp_cp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p)
{
	uint8_t cmd_buf[sizeof(struct osdp_data) + sizeof(union cmd_all)];
	struct osdp_data *cmd = (struct osdp_data *)cmd_buf;

	cmd->id = CMD_COMSET;
	cmd->len = sizeof(struct osdp_data) + sizeof(struct osdp_cmd_comset);
	memcpy(cmd->data, p, sizeof(struct osdp_cmd_comset));

	if (cp_enqueue_command(to_pd(ctx, pd), cmd) != 0) {
		LOG_W(TAG "CMD_BUZ enqueue error!");
		return -1;
	}
	return 0;
}
