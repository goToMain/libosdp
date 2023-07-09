/*
 * Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <unistd.h>
#include <osdp.hpp>

int sample_pd_send_func(void *data, uint8_t *buf, int len)
{
	(void)(data);
	(void)(buf);

	// Fill these

	return len;
}

int sample_pd_recv_func(void *data, uint8_t *buf, int len)
{
	(void)(data);
	(void)(buf);
	(void)(len);

	// Fill these

	return 0;
}

int pd_command_handler(void *self, struct osdp_cmd *cmd)
{
	(void)(self);

	std::cout << "PD: CMD: " << cmd->id << std::endl;
	return 0;
}

osdp_pd_info_t info_pd = {
	.name = "pd[101]",
	.baud_rate = 9600,
	.address = 101,
	.flags = 0,
	.id = {
		.version = 1,
		.model = 153,
		.vendor_code = 31337,
		.serial_number = 0x01020304,
		.firmware_version = 0x0A0B0C0D,
	},
	.cap = (struct osdp_pd_cap []) {
		{
			.function_code = OSDP_PD_CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 1
		},
		{
			.function_code = OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,
			.compliance_level = 1,
			.num_items = 1
		},
		{ static_cast<uint8_t>(-1), 0, 0 } /* Sentinel */
	},
	.channel = {
		.data = nullptr,
		.id = 0,
		.recv = sample_pd_recv_func,
		.send = sample_pd_send_func,
		.flush = nullptr
	},
	.scbk = nullptr,
};

int main()
{
	OSDP::PeripheralDevice pd;

	pd.logger_init("osdp::pd", OSDP_LOG_DEBUG, NULL);

	pd.setup(&info_pd);

	pd.set_command_callback(pd_command_handler);

	while (1) {
		pd.refresh();

		// your application code.
		usleep(1000);
	}

	return 0;
}
