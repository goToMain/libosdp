/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>

#include <osdp.h>
#include "test.h"
#include "osdp_cp_private.h"

int pd_phy_state_update(struct osdp_pd *pd);
int cp_state_update(struct osdp_pd *pd);

struct test_mixed {
	struct osdp *cp_ctx;
	struct osdp *pd_ctx;
} test_data;

uint8_t test_mixed_cp_to_pd_buf[128];
int test_mixed_cp_to_pd_buf_length;

uint8_t test_mixed_pd_to_cp_buf[128];
int test_mixed_pd_to_cp_buf_length;

int test_mixed_cp_fsm_send(uint8_t * buf, int len)
{
	memcpy(test_mixed_cp_to_pd_buf, buf, len);
	test_mixed_cp_to_pd_buf_length = len;
	osdp_dump("CP Send", buf, len);
	return len;
}

int test_mixed_cp_fsm_receive(uint8_t * buf, int len)
{
	int ret = test_mixed_pd_to_cp_buf_length;

	if (ret) {
		memcpy(buf, test_mixed_pd_to_cp_buf, ret);
		test_mixed_pd_to_cp_buf_length = 0;
	}
	osdp_dump("PD Recv", buf, ret);
	return ret;
}

int test_mixed_pd_fsm_send(uint8_t * buf, int len)
{
	memcpy(test_mixed_pd_to_cp_buf, buf, len);
	test_mixed_pd_to_cp_buf_length = len;
	return len;
}

int test_mixed_pd_fsm_receive(uint8_t * buf, int len)
{
	int ret = test_mixed_cp_to_pd_buf_length;

	if (test_mixed_cp_to_pd_buf_length) {
		memcpy(buf, test_mixed_cp_to_pd_buf,
		       test_mixed_cp_to_pd_buf_length);
		test_mixed_cp_to_pd_buf_length = 0;
	}

	return ret;
}

int test_mixed_fsm_setup(struct test *t)
{
	/* mock application data */
	osdp_pd_info_t info_cp = {
		.address = 101,
		.baud_rate = 9600,
		.init_flags = 0,
		.send_func = test_mixed_cp_fsm_send,
		.recv_func = test_mixed_cp_fsm_receive
	};
	test_data.cp_ctx = (struct osdp *) osdp_cp_setup(1, &info_cp);
	if (test_data.cp_ctx == NULL) {
		printf("   cp init failed!\n");
		return -1;
	}

	struct pd_cap cap[] = {
		{
			.function_code = CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 1
		}, {
			.function_code = CAP_COMMUNICATION_SECURITY,
			.compliance_level = 1,
			.num_items = 1
		},
		OSDP_PD_CAP_SENTINEL
	};
	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 9600,
		.init_flags = 0,
		.send_func = test_mixed_pd_fsm_send,
		.recv_func = test_mixed_pd_fsm_receive,
		.id = {
		       .version = 1,
		       .model = 153,
		       .vendor_code = 31337,
		       .serial_number = 0x01020304,
		       .firmware_version = 0x0A0B0C0D,
		       },
		.cap = cap,
	};
	test_data.pd_ctx = (struct osdp *) osdp_pd_setup(&info_pd);
	if (test_data.pd_ctx == NULL) {
		printf("   pd init failed!\n");
		osdp_cp_teardown((osdp_cp_t *) test_data.cp_ctx);
		return -1;
	}
	osdp_set_log_level(LOG_DEBUG);
	t->mock_data = (void *)&test_data;
	return 0;
}

void test_mixed_fsm_teardown(struct test *t)
{
	struct test_mixed *p = t->mock_data;

	osdp_cp_teardown((osdp_cp_t *) p->cp_ctx);
	osdp_pd_teardown((osdp_pd_t *) p->pd_ctx);
}

void run_mixed_fsm_tests(struct test *t)
{
	int result = TRUE;
	uint32_t count = 0;
	struct test_mixed *p;

	printf("\nStarting CP - PD phy layer mixed tests\n");

	if (test_mixed_fsm_setup(t))
		return;

	p = t->mock_data;

	printf("    -- executing CP - PD mixed tests\n");
	while (1) {
		cp_state_update(to_current_pd(p->cp_ctx));
		pd_phy_state_update(to_current_pd(p->pd_ctx));

		if (to_current_pd(p->cp_ctx)->state == CP_STATE_OFFLINE) {
			printf("    -- CP went offline!\n");
			result = FALSE;
			break;
		}
		if (to_current_pd(p->pd_ctx)->phy_state == 2) {
			printf("    -- PD phy state error!\n");
			result = FALSE;
			break;
		}
		if (count++ > 300)
			break;
		usleep(1000);
	}
	printf("    -- CP - PD mixed tests complete\n");

	TEST_REPORT(t, result);

	test_mixed_fsm_teardown(t);
}
