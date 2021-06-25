/*
 * Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <osdp.h>

#include "osdp_common.h"
#include "test.h"

uint8_t test_cp_to_pd_buf[128];
int test_cp_to_pd_buf_length;

uint8_t test_pd_to_cp_buf[128];
int test_pd_to_cp_buf_length;

int test_mock_cp_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	memcpy(test_cp_to_pd_buf, buf, len);
	test_cp_to_pd_buf_length = len;
	return len;
}

int test_mock_cp_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	int ret = test_pd_to_cp_buf_length;

	ARG_UNUSED(len);

	if (ret) {
		memcpy(buf, test_pd_to_cp_buf, ret);
		test_pd_to_cp_buf_length = 0;
	}
	return ret;
}

int test_mock_pd_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	memcpy(test_pd_to_cp_buf, buf, len);
	test_pd_to_cp_buf_length = len;
	return len;
}

int test_mock_pd_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	int ret = test_cp_to_pd_buf_length;

	if (test_cp_to_pd_buf_length) {
		memcpy(buf, test_cp_to_pd_buf,
		       test_cp_to_pd_buf_length);
		test_cp_to_pd_buf_length = 0;
	}

	return ret;
}

int test_setup_devices(struct test *t, osdp_t **cp, osdp_t **pd)
{
	osdp_logger_init(t->loglevel, printf);

	uint8_t scbk[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	/* mock application data */
	osdp_pd_info_t info_cp = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = test_mock_cp_send,
		.channel.recv = test_mock_cp_receive,
		.channel.flush = NULL,
		.scbk = scbk,
	};

	*cp = osdp_cp_setup(1, &info_cp, NULL);
	if (*cp == NULL) {
		printf("   cp init failed!\n");
		return -1;
	}

	struct osdp_pd_cap cap[] = {
		{ .function_code = OSDP_PD_CAP_READER_LED_CONTROL,
		  .compliance_level = 1,
		  .num_items = 1 },
		{ .function_code = OSDP_PD_CAP_COMMUNICATION_SECURITY,
		  .compliance_level = 1,
		  .num_items = 1 },
		{ -1, 0, 0 }
	};
	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = cap,
		.channel.data = NULL,
		.channel.send = test_mock_pd_send,
		.channel.recv = test_mock_pd_receive,
		.scbk = scbk,
	};

	*pd = (struct osdp *)osdp_pd_setup(&info_pd);
	if (*pd == NULL) {
		printf(SUB_1 "pd init failed!\n");
		osdp_cp_teardown(*cp);
		return -1;
	}

	return 0;
}

void test_start(struct test *t)
{
	printf("\n");
	printf("------------------------------------------\n");
	printf("            OSDP - Unit Tests             \n");
	printf("------------------------------------------\n");
	printf("\n");

	t->tests = 0;
	t->success = 0;
	t->failure = 0;
	t->loglevel = LOG_INFO;
}

int test_end(struct test *t)
{
	printf("\n");
	printf("------------------------------------------\n");
	printf("Tests: %d\tSuccess: %d\tFailure: %d\n", t->tests, t->success,
	       t->failure);
	printf("\n");

	if (t->tests != t->success)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	struct test t;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	srand(time(NULL));

	test_start(&t);

	run_cp_phy_tests(&t);

	run_cp_phy_fsm_tests(&t);

	run_cp_fsm_tests(&t);

	run_mixed_fsm_tests(&t);

	run_osdp_commands_tests(&t);

	return test_end(&t);
}
