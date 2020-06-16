/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <osdp.h>
#include "test.h"

int cp_phy_state_update(struct osdp_pd *pd);
void cp_enqueue_command(struct osdp_pd *p, struct osdp_cmd *cmd);
int cp_alloc_command(struct osdp *ctx, struct osdp_cmd **cmd);

int phy_fsm_resp_offset = 0;

int test_cp_phy_fsm_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	uint8_t cmd_poll[] =
	    { 0xff, 0x53, 0x65, 0x08, 0x00, 0x04, 0x60, 0x60, 0x90 };
	uint8_t cmd_id[] =
	    { 0xff, 0x53, 0x65, 0x09, 0x00, 0x05, 0x61, 0x00, 0xe9, 0x4d };

	switch (phy_fsm_resp_offset) {
	case 0:
		if (memcmp(buf, cmd_poll, len) != 0) {
			printf("    -- poll buf Mismatch!\n");
			osdp_dump("Attempt to send", buf, len);
		}
		break;
	case 1:
		if (memcmp(buf, cmd_id, len) != 0) {
			printf("    -- id buf Mismatch!\n");
			osdp_dump("Attempt to send", buf, len);
		}
		break;
	}
	return len;
}

int test_cp_phy_fsm_receive(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	uint8_t resp_ack[] = {
		0xff, 0x53, 0xe5, 0x08, 0x00, 0x04, 0x40, 0xd2, 0x96
	};
	uint8_t resp_id[] = {
		0xff, 0x53, 0xe5, 0x14, 0x00, 0x05, 0x45, 0xa1, 0xa2,
		0xa3, 0xb1, 0xc1, 0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2,
		0xe3, 0x99, 0xa2
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
		.channel.flush = NULL
	};
	struct osdp *ctx = (struct osdp *) osdp_cp_setup(1, &info, NULL);
	if (ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	// osdp_set_log_level(LOG_DEBUG);
	set_current_pd(ctx, 0);
	t->mock_data = (void *)ctx;
	return 0;
}

void test_cp_phy_fsm_teardown(struct test *t)
{
	osdp_cp_teardown(t->mock_data);
}

void run_cp_phy_fsm_tests(struct test *t)
{
	int ret=-128, result = TRUE;
	struct osdp_cmd *cmd_poll;
	struct osdp_cmd *cmd_id;
	struct osdp *ctx;
	struct osdp_pd *p;

	printf("\nStarting CP fsm state tests\n");

	if (test_cp_phy_fsm_setup(t)) {
		printf("    -- error failed to setup cp_phy\n");
		return;
	}

	ctx = t->mock_data;
	p = to_current_pd(ctx);

	cp_alloc_command(ctx, &cmd_poll);
	cp_alloc_command(ctx, &cmd_id);

	cmd_poll->id = CMD_POLL;
	cmd_id->id = CMD_ID;

	cp_enqueue_command(p, cmd_poll);
	cp_enqueue_command(p, cmd_id);

	printf("    -- executing test_cp_phy_fsm()\n");
	while (result) {
		ret = cp_phy_state_update(p);
		if (ret != 1 && ret != 2)
			break;
		/* continue when in command and between commands continue */
	}

	if (p->id.vendor_code != 0x00a3a2a1 ||
	    p->id.model != 0xb1 ||
	    p->id.version != 0xc1 ||
	    p->id.serial_number != 0xd4d3d2d1 ||
	    p->id.firmware_version != 0x00e1e2e3) {
		printf( "    -- error ID mismatch! 0x%04x 0x%02x"
			"0x%02x 0x%04x 0x%04x\n", p->id.vendor_code, p->id.model,
			p->id.version, p->id.serial_number, p->id.firmware_version);
		result = FALSE;
	}

	TEST_REPORT(t, result);

	printf("    -- test_cp_phy_fsm() complete -- %d\n", ret);

	test_cp_phy_fsm_teardown(t);
}
