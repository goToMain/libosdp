/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>

#include "rs232.h"
#include "uart.h"

int uart_num = -1;

int uart_write(uint8_t *buf, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		while (RS232_SendByte(uart_num, buf[i]));
	}
	return len;
}

int uart_read(uint8_t *buf, int maxLen)
{
	return RS232_PollComport(uart_num, buf, maxLen);
}

int uart_init(const char *dev, int baud_rate)
{
	if (uart_num != -1)
		return uart_num;

	uart_num = RS232_GetPortnr(dev);

	if (uart_num < 0) {
		printf("invalid device name %s\n", dev);
		return -1;
	}

	if (RS232_OpenComport(uart_num, baud_rate, "8N1", 0)) {
		printf("failed to open %s\n", dev);
		return -1;
	}

	return uart_num;
}
