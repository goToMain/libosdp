/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

	OSDP::ControlPanel cp(1, info, nullptr);
	cp.set_log_level(7);

	while (1) {
		// your application code.

		cp.refresh();
		usleep(1000);
	}

	return 0;
}
