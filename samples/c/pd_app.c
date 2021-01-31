/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
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

int pd_command_handler(void *arg, struct osdp_cmd *cmd)
{
	(void)(arg);

	printf("PD: CMD: %d\n", cmd->id);
	return 0;
}

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
		{ (uint8_t)-1, 0, 0 } /* Sentinel */
	}
};

int main()
{
	struct osdp_t *ctx;

	ctx = osdp_pd_setup(&info_pd, NULL);
	if (ctx == NULL) {
		printf("pd init failed!\n");
		return -1;
	}

	osdp_pd_set_command_callback(ctx, pd_command_handler, NULL);

	while (1) {
		osdp_pd_refresh(ctx);

		// your application code.
		usleep(1000);
	}

	return 0;
}
