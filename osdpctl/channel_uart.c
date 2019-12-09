/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "rs232.h"
#include "common.h"

struct channel_uart_s {
        int port_id;
};

int channel_uart_send(void *data, uint8_t *buf, int len)
{
	int i;
	struct channel_uart_s *ctx = data;

	for (i = 0; i < len; i++) {
		while (RS232_SendByte(ctx->port_id, buf[i]));
	}

	return len;
}

int channel_uart_recv(void *data, uint8_t *buf, int maxLen)
{
	struct channel_uart_s *ctx = data;

	return RS232_PollComport(ctx->port_id, buf, maxLen);
}

int channel_uart_setup(void **data, struct config_pd_s *c)
{
	struct channel_uart_s *ctx;

	ctx = malloc(sizeof(struct channel_uart_s));
	if (ctx == NULL) {
		printf("Failed at alloc for uart channel\n");
		return -1;
	}

	ctx->port_id = RS232_GetPortnr(c->channel_device);
	if (ctx->port_id < 0) {
		printf("Error: invalid uart device %s\n",
		       c->channel_device);
		return -1;
	}
	if (RS232_OpenComport(ctx->port_id, c->channel_speed, "8N1", 0)) {
		printf("Error: failed to open device %s\n",
		       c->channel_device);
		return -1;
	}
	*data = (void *)ctx;

	return 0;
}

void channel_uart_teardown(void *data)
{
	struct channel_uart_s *ctx = data;

	RS232_CloseComport(ctx->port_id);
	free(ctx);
}

struct channel_ops_s channel_uart = {
	.send = channel_uart_send,
	.recv = channel_uart_recv,
	.flush = NULL,
	.setup = channel_uart_setup,
	.teardown = channel_uart_teardown
};
