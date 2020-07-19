/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "common.h"

int channel_setup(struct config_pd_s *p)
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
	case CONFIG_CHANNEL_TYPE_CUSTOM:
		channel = &channel_custom;
		break;
	default:
		printf("Error: invalid channel\n");
		return -1;
	}
	if (channel->setup(&ctx, p))
		return -1;
	p->channel.data = ctx;
	p->channel.recv = channel->recv;
	p->channel.send = channel->send;
	p->channel.flush = channel->flush;

	return 0;
}

void channel_teardown(struct config_pd_s *p)
{
	switch (p->channel_type) {
	case CONFIG_CHANNEL_TYPE_UART:
		channel_uart.teardown(p->channel.data);
		break;
	case CONFIG_CHANNEL_TYPE_MSGQ:
		channel_msgq.teardown(p->channel.data);
		break;
	}
}
