/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <osdp.h>

#include "test.h"

extern int (*test_state_update)(struct osdp_pd *);
extern int (*test_cp_phy_state_update)(struct osdp_pd *);
extern void (*test_cp_cmd_enqueue)(struct osdp_pd *, struct osdp_cmd *);
extern struct osdp_cmd *(*test_cp_cmd_alloc)(struct osdp_pd *);
extern const int CP_ERR_CAN_YIELD;
extern const int CP_ERR_INPROG;

int phy_fsm_resp_offset = 0;

int test_cp_phy_fsm_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	uint8_t cmd_poll[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0x65, 0x08, 0x00, 0x04, 0x60, 0x60, 0x90
	};
	uint8_t cmd_id[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0x65, 0x09, 0x00, 0x05, 0x61, 0x00, 0xe9, 0x4d
	};

	switch (phy_fsm_resp_offset) {
	case 0:
		if (memcmp(buf, cmd_poll, len) != 0) {
			printf(SUB_1 "poll buf Mismatch!\n");
			CHECK_ARRAY(buf, len, cmd_poll);
		}
		break;
	case 1:
		if (memcmp(buf, cmd_id, len) != 0) {
			printf(SUB_1 "id buf Mismatch!\n");
			CHECK_ARRAY(buf, len, cmd_id);
		}
		break;
	}
	return len;
}

int test_cp_phy_fsm_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	uint8_t resp_ack[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0xe5, 0x08, 0x00, 0x04, 0x40, 0xd2, 0x96
	};
	uint8_t resp_id[] = {
#ifndef CONFIG_OSDP_SKIP_MARK_BYTE
		0xff,
#endif
		0x53, 0xe5, 0x14, 0x00, 0x05, 0x45, 0xa1, 0xa2, 0xa3, 0xb1,
		0xc1, 0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2, 0xe3, 0x99, 0xa2
	};

	ARG_UNUSED(len);

	switch (phy_fsm_resp_offset) {
	case 0:
		memcpy(buf, resp_ack, sizeof(resp_ack));
		phy_fsm_resp_offset++;
		return sizeof(resp_ack);
	case 1:
		memcpy(buf, resp_id, sizeof(resp_id));
		phy_fsm_resp_offset++;
		return sizeof(resp_id);
	}
	return 0;
}

int test_cp_phy_fsm_setup(struct test *t)
{
	/* mock application data */
	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = test_cp_phy_fsm_send,
		.channel.recv = test_cp_phy_fsm_receive,
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
	t->mock_data = (void *)ctx;
	return 0;
}

void test_cp_phy_fsm_teardown(struct test *t)
{
	osdp_cp_teardown(t->mock_data);
}

void run_cp_phy_fsm_tests(struct test *t)
{
	int ret = -128, result = true;
	struct osdp_cmd *cmd_poll;
	struct osdp_cmd *cmd_id;
	struct osdp *ctx;
	struct osdp_pd *p;

	printf("\nBeginning CP fsm state tests\n");

	if (test_cp_phy_fsm_setup(t)) {
		printf(SUB_1 "error failed to setup cp_phy\n");
		return;
	}

	ctx = t->mock_data;
	p = GET_CURRENT_PD(ctx);

	cmd_poll = test_cp_cmd_alloc(p);
	cmd_id = test_cp_cmd_alloc(p);

	if (cmd_poll == NULL || cmd_id == NULL) {
		printf(SUB_1 "cmd alloc failed\n");
		return;
	}

	cmd_poll->id = CMD_POLL;
	cmd_id->id = CMD_ID;

	test_cp_cmd_enqueue(p, cmd_poll);
	test_cp_cmd_enqueue(p, cmd_id);

	printf(SUB_1 "executing test_cp_phy_fsm()\n");
	while (result) {
		ret = test_cp_phy_state_update(p);
		if (ret != CP_ERR_CAN_YIELD && ret != CP_ERR_INPROG)
			break;
		/* continue when in command and between commands continue */
	}
	printf(SUB_1 "of text loop\n");
	if (p->id.vendor_code != 0x00a3a2a1 || p->id.model != 0xb1 ||
	    p->id.version != 0xc1 || p->id.serial_number != 0xd4d3d2d1 ||
	    p->id.firmware_version != 0x00e1e2e3) {
		printf(SUB_1 "ID mismatch! VC:0x%04x MODEL:0x%02x "
			     "VER:0x%02x SERIAL:0x%04x FW_VER:0x%04x\n",
		       p->id.vendor_code, p->id.model, p->id.version,
		       p->id.serial_number, p->id.firmware_version);
		result = false;
	}

	TEST_REPORT(t, result);

	printf(SUB_1 "Finished CP fsm state tests -- %s!\n",
	       result ? "success" : "failure");

	test_cp_phy_fsm_teardown(t);
}
