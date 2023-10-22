/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <osdp.h>
#include "test.h"

extern int (*test_state_update)(struct osdp_pd *);

int test_fsm_resp = 0;

int test_cp_fsm_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
	int cmd_id_offset = OSDP_CMD_ID_OFFSET + 1;
#else
	int cmd_id_offset = OSDP_CMD_ID_OFFSET;
#endif

	switch (buf[cmd_id_offset]) {
	case 0x60:
		test_fsm_resp = 1;
		break;
	case 0x61:
		test_fsm_resp = 2;
		break;
	case 0x62:
		test_fsm_resp = 3;
		break;
	default:
		printf(SUB_1 "invalid ID:0x%02x\n", buf[cmd_id_offset

		]);
	}
	return len;
}

int test_cp_fsm_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	uint8_t resp_id[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0xe5, 0x14, 0x00, 0x04, 0x45, 0xa1, 0xa2, 0xa3, 0xb1,
		0xc1, 0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2, 0xe3, 0xf8, 0xd9
	};
	uint8_t resp_cap[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0xe5, 0x0b, 0x00, 0x05, 0x46, 0x04, 0x04, 0x01, 0xb3, 0xec
	};
	uint8_t resp_ack[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0xe5, 0x08, 0x00, 0x06, 0x40, 0xb0, 0xf0
	};

	ARG_UNUSED(len);

	switch (test_fsm_resp) {
	case 1:
		memcpy(buf, resp_ack, sizeof(resp_ack));
		return sizeof(resp_ack);
	case 2:
		memcpy(buf, resp_id, sizeof(resp_id));
		return sizeof(resp_id);
	case 3:
		memcpy(buf, resp_cap, sizeof(resp_cap));
		return sizeof(resp_cap);
	}
	return -1;
}

int test_cp_fsm_setup(struct test *t)
{
	/* mock application data */
	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = test_cp_fsm_send,
		.channel.recv = test_cp_fsm_receive,
		.channel.flush = NULL,
		.scbk = NULL,
	};
	osdp_logger_init3("osdp::cp", t->loglevel, NULL);
	struct osdp *ctx = (struct osdp *)osdp_cp_setup2(1, &info);
	if (ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	SET_CURRENT_PD(ctx, 0);
	SET_FLAG(GET_CURRENT_PD(ctx), PD_FLAG_SKIP_SEQ_CHECK);
	t->mock_data = (void *)ctx;
	return 0;
}

void test_cp_fsm_teardown(struct test *t)
{
	osdp_cp_teardown(t->mock_data);
}

void run_cp_fsm_tests(struct test *t)
{
	int result = true;
	uint32_t count = 0;
	struct osdp *ctx;

	printf("\nStarting CP Phy state tests\n");

	if (test_cp_fsm_setup(t))
		return;

	ctx = t->mock_data;

	printf(SUB_1 "executing state_update()\n");
	while (1) {
		test_state_update(GET_CURRENT_PD(ctx));

		if (GET_CURRENT_PD(ctx)->state == OSDP_CP_STATE_OFFLINE) {
			printf(SUB_2 "state_update() CP went offline\n");
			result = false;
			break;
		}
		if (count++ > 300)
			break;
		usleep(1000);
	}
	printf(SUB_1 "state_update test %s\n", result ? "succeeded" : "failed");

	TEST_REPORT(t, result);

	test_cp_fsm_teardown(t);
}

// unnecessary
