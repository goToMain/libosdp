/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <unistd.h>
#include <osdp.hpp>

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

osdp_pd_info_t pd_info[] = {
	{
		.name = "pd[101]",
		.baud_rate = 115200,
		.address = 101,
		.flags = 0,
		.id = {},
		.cap = nullptr,
		.channel = {
			.data = nullptr,
			.id = 0,
			.recv = sample_cp_recv_func,
			.send = sample_cp_send_func,
			.flush = nullptr
		},
		.scbk = nullptr,
	}
};

int main()
{
	OSDP::ControlPanel cp;

	cp.logger_init("osdp::cp", OSDP_LOG_DEBUG, NULL);

	cp.setup(1, pd_info);

	while (1) {
		// your application code.

		cp.refresh();
		usleep(1000);
	}

	return 0;
}
