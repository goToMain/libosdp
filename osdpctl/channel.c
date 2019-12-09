/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "common.h"

int channel_setup(struct osdp_channel *c, struct config_pd_s *p)
{
	void *ctx;
	struct channel_ops_s *channel;

	switch (p->channel_type) {
	case CONFIG_CHANNEL_TYPE_UART:
		channel = &channel_uart;
		break;
	case CONFIG_CHANNEL_TYPE_MSGQ:
		channel = &channel_msgq;
		break;
	default:
		printf("Error: invalid channel\n");
		return -1;
	}
	if (channel->setup(&ctx, p))
		return -1;
	c->data = ctx;
	c->recv = channel->recv;
	c->send = channel->send;
	c->flush = channel->flush;

	return 0;
}

void channel_teardown(struct osdp_channel *c, struct config_pd_s *p)
{
	switch (p->channel_type) {
	case CONFIG_CHANNEL_TYPE_UART:
		channel_uart.teardown(c->data);
		break;
	case CONFIG_CHANNEL_TYPE_MSGQ:
		channel_msgq.teardown(c->data);
		break;
	}
}
