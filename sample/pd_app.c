/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <osdp.h>

enum osdp_pd_e {
	OSDP_PD_1,
	OSDP_PD_2,
	OSDP_PD_SENTINEL,
};

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

int main()
{
	struct osdp_t *ctx;
	struct osdp_cmd cmd;
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
		.flags = 0,
		.channel.send = sample_pd_send_func,
		.channel.recv = sample_pd_recv_func,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = cap,
	};

	ctx = osdp_pd_setup(&info_pd, NULL);
	if (ctx == NULL) {
		printf("pd init failed!\n");
		return -1;
	}

	while (1) {
		osdp_pd_refresh(ctx);
		if (osdp_pd_get_cmd(ctx, &cmd) == 0) {
			// do something with command.
		}

		// your application code.
		usleep(1000);
	}

	return 0;
}
