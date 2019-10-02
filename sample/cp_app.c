/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <unistd.h>
#include <osdp.h>
#include "uart.h"

int main()
{
	osdp_pd_info_t info[] = {
		{
			.address = 101,
			.baud_rate = 115200,
			.init_flags = 0,
			.send_func = uart_write,
			.recv_func = uart_read
		},
	};

	if(uart_init("/dev/ttyUSB1", 115200) < 0)
		return -1;

	osdp_cp_t *ctx = osdp_cp_setup(1, info);
	osdp_set_log_level(7);

	if (ctx == NULL)
	{
		printf("cp init failed!\n");
		return -1;
	}

	while (1)
	{
		// your application code.

		osdp_cp_refresh(ctx);
		usleep(1000);
	}
	return 0;
}
