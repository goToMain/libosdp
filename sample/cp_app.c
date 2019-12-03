/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <osdp.h>

int sample_cp_send_func(uint8_t *buf, int len)
{
	return len;
}

int sample_cp_recv_func(uint8_t *buf, int len)
{
	return 0;
}

int main()
{
	osdp_pd_info_t info[] = {
		{
			.address = 101,
			.baud_rate = 115200,
			.flags = 0,
			.send_func = sample_cp_send_func,
			.recv_func = sample_cp_recv_func
		},
	};

	osdp_cp_t *ctx = osdp_cp_setup(1, info, NULL);
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
