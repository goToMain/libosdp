/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <osdp.h>

enum osdp_pd_e {
	OSDP_PD_1,
	OSDP_PD_2,
	OSDP_PD_SENTINEL,
};

int sample_pd_send_func(uint8_t *buf, int len)
{
	return len;
}

int sample_pd_recv_func(uint8_t *buf, int len)
{
	return 0;
}

int main()
{
	struct osdp_pd *ctx;
	struct pd_cap cap[] = {
		{
			.function_code = CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 1
		},
		{
			.function_code = CAP_READER_AUDIBLE_OUTPUT,
			.compliance_level = 1,
			.num_items = 1
		},
		OSDP_PD_CAP_SENTINEL
	};
	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 9600,
		.init_flags = 0,
		.send_func = sample_pd_send_func,
		.recv_func = sample_pd_recv_func,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = cap,
	};

	ctx = osdp_pd_setup(&info_pd);
	if (ctx == NULL) {
		printf("pd init failed!\n");
		return -1;
	}

	while (1) {
		// your application code.

		osdp_pd_refresh(ctx);
	}

	return 0;
}
