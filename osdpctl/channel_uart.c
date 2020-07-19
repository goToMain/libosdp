/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "serial.h"
#include "common.h"

int channel_uart_send(void *data, uint8_t *buf, int len)
{
	return serial_write(data, (unsigned char *)buf, len);
}

int channel_uart_recv(void *data, uint8_t *buf, int maxLen)
{
	return serial_read(data, (unsigned char *)buf, maxLen);
}

void channel_uart_flush(void *data)
{
	serial_flush(data);
}

int channel_uart_setup(void **data, struct config_pd_s *c)
{
	struct serial *ctx;

	ctx = serial_open(c->channel_device, c->channel_speed, "8N1");
	if (ctx == NULL) {
		printf("Error: failed to open device %s\n",
		       c->channel_device);
		return -1;
	}
	*data = (void *)ctx;
	return 0;
}

void channel_uart_teardown(void *data)
{
	serial_close(data);
}

struct channel_ops_s channel_uart = {
	.send = channel_uart_send,
	.recv = channel_uart_recv,
	.flush = channel_uart_flush,
	.setup = channel_uart_setup,
	.teardown = channel_uart_teardown
};
