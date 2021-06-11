/*
 * Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <osdp.h>
#include "test.h"

extern int (*test_state_update)(struct osdp_pd *);
extern void (*test_osdp_pd_update)(struct osdp_pd *pd);

struct test_mixed {
	struct osdp *cp_ctx;
	struct osdp *pd_ctx;
} test_data;

uint8_t test_mixed_cp_to_pd_buf[128];
int test_mixed_cp_to_pd_buf_length;

uint8_t test_mixed_pd_to_cp_buf[128];
int test_mixed_pd_to_cp_buf_length;

int test_mixed_cp_fsm_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	memcpy(test_mixed_cp_to_pd_buf, buf, len);
	test_mixed_cp_to_pd_buf_length = len;
	return len;
}

int test_mixed_cp_fsm_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	int ret = test_mixed_pd_to_cp_buf_length;

	ARG_UNUSED(len);

	if (ret) {
		memcpy(buf, test_mixed_pd_to_cp_buf, ret);
		test_mixed_pd_to_cp_buf_length = 0;
	}
	return ret;
}

int test_mixed_pd_fsm_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	memcpy(test_mixed_pd_to_cp_buf, buf, len);
	test_mixed_pd_to_cp_buf_length = len;
	return len;
}

int test_mixed_pd_fsm_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	int ret = test_mixed_cp_to_pd_buf_length;

	if (test_mixed_cp_to_pd_buf_length) {
		memcpy(buf, test_mixed_cp_to_pd_buf,
		       test_mixed_cp_to_pd_buf_length);
		test_mixed_cp_to_pd_buf_length = 0;
	}

	return ret;
}

uint8_t master_key[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

int test_mixed_fsm_setup(struct test *t)
{
	/* mock application data */
	osdp_pd_info_t info_cp = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = test_mixed_cp_fsm_send,
		.channel.recv = test_mixed_cp_fsm_receive,
		.channel.flush = NULL,
		.scbk = NULL,
	};
	test_data.cp_ctx = (struct osdp *) osdp_cp_setup(1, &info_cp, master_key);
	if (test_data.cp_ctx == NULL) {
		printf("   cp init failed!\n");
		return -1;
	}

	struct osdp_pd_cap cap[] = {
		{
			.function_code = OSDP_PD_CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 1
		}, {
			.function_code = OSDP_PD_CAP_COMMUNICATION_SECURITY,
			.compliance_level = 1,
			.num_items = 1
		},
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
		.channel.send = test_mixed_pd_fsm_send,
		.channel.recv = test_mixed_pd_fsm_receive,
		.scbk = NULL,
	};
	osdp_logger_init(t->loglevel, printf);
	test_data.pd_ctx = (struct osdp *) osdp_pd_setup(&info_pd);
	if (test_data.pd_ctx == NULL) {
		printf(SUB_1 "pd init failed!\n");
		osdp_cp_teardown((osdp_t *) test_data.cp_ctx);
		return -1;
	}
	t->mock_data = (void *)&test_data;
	return 0;
}

void test_mixed_fsm_teardown(struct test *t)
{
	struct test_mixed *p = t->mock_data;

	osdp_cp_teardown((osdp_t *) p->cp_ctx);
	osdp_pd_teardown((osdp_t *) p->pd_ctx);
}

void run_mixed_fsm_tests(struct test *t)
{
	int result = true;
	struct test_mixed *p;
	struct osdp_pd *pd_cp, *pd_pd;
	int64_t start;

	printf("\nBegin CP - PD phy layer mixed tests\n");

	printf(SUB_1 "setting up OSDP devices\n");

	if (test_mixed_fsm_setup(t))
		return;

	p = t->mock_data;

	printf(SUB_1 "executing CP - PD mixed tests\n");
	start = osdp_millis_now();
	pd_cp = GET_CURRENT_PD(p->cp_ctx);
	pd_pd = GET_CURRENT_PD(p->pd_ctx);
	while (1) {
		test_state_update(pd_cp);
		test_osdp_pd_update(pd_pd);
		if (osdp_get_sc_status_mask(p->cp_ctx))
			break;
		if (pd_cp->state == OSDP_CP_STATE_OFFLINE) {
			printf(SUB_1 "CP went offline!\n");
			result = false;
			break;
		}
		if (pd_pd->state == OSDP_PD_STATE_ERR) {
			printf(SUB_1 "PD state error!\n");
			result = false;
			break;
		}
		if (osdp_millis_since(start) > 2 * 1000) {
			printf(SUB_1 "test timout!\n");
			result = false;
			break;
		}
	}
	printf(SUB_1 "CP - PD mixed tests complete\n");

	TEST_REPORT(t, result);

	test_mixed_fsm_teardown(t);
}
