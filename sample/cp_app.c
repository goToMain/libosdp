/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <osdp.h>

int sample_cp_send_func(void *data, uint8_t *buf, int len)
{
	(void)(data);
	(void)(buf);

	// Fill these

	return len;
}

int sample_cp_recv_func(void *data, uint8_t *buf, int len)
{
	(void)(data);
	(void)(buf);
	(void)(len);

	// Fill these

	return 0;
}

int main()
{
	osdp_pd_info_t info[] = {
		{
			.address = 101,
			.baud_rate = 115200,
			.flags = 0,
			.channel.send = sample_cp_send_func,
			.channel.recv = sample_cp_recv_func
		},
	};

	osdp_t *ctx = osdp_cp_setup(1, info, NULL);
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
