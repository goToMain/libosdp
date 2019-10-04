/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include "test.h"
#include "osdp_cp_private.h"

int phy_build_packet_head(struct osdp_pd *pd, int id, uint8_t * buf, int maxlen);
int phy_build_packet_tail(struct osdp_pd *pd, uint8_t * buf, int len, int maxlen);
int phy_decode_packet(struct osdp_pd *pd, uint8_t * buf, int len);

int cp_build_command(struct osdp_pd *pd, struct osdp_data *cmd, uint8_t * buf, int blen);
int cp_decode_response(struct osdp_pd *pd, uint8_t * buf, int len);

int cp_enqueue_command(struct osdp_pd *pd, struct osdp_data *c);
int cp_dequeue_command(struct osdp_pd *pd, int readonly, uint8_t * cmd_buf, int maxlen);

static int test_cp_build_packet(struct osdp_pd *p, uint8_t * buf, int len, int maxlen)
{
	int cmd_len;
	uint8_t cmd_buf[128];

	if (len > 128) {
		osdp_log(LOG_NOTICE, "cmd_buf len err - %d/%d", len, 128);
		return -1;
	}
	cmd_len = len;
	memcpy(cmd_buf, buf, len);

	if ((len = phy_build_packet_head(p, 0, buf, maxlen)) < 0) {
		osdp_log(LOG_ERR, "failed to phy_build_packet_head");
		return -1;
	}
	memcpy(buf + len, cmd_buf, cmd_len);
	len += cmd_len;
	if ((len = phy_build_packet_tail(p, buf, len, maxlen)) < 0) {
		osdp_log(LOG_ERR, "failed to build command");
		return -1;
	}
	return len;
}

int test_cp_build_packet_poll(struct osdp * ctx)
{
	int len;
	struct osdp_pd *p = to_current_pd(ctx);
	uint8_t packet[512] = { CMD_POLL };
	uint8_t expected[] = { 0xff, 0x53, 0x65, 0x08, 0x00,
		0x04, 0x60, 0x60, 0x90
	};

	printf("Testing cp_build_packet(CMD_POLL) -- ");
	if ((len = test_cp_build_packet(p, packet, 1, 512)) < 0) {
		printf("error!\n");
	}
	CHECK_ARRAY(packet, len, expected);
	printf("success!\n");
	return 0;
}

int test_cp_build_packet_id(struct osdp * ctx)
{
	int len;
	struct osdp_pd *p = to_current_pd(ctx);
	uint8_t packet[512] = { CMD_ID, 0x00 };
	uint8_t expected[] = { 0xff, 0x53, 0x65, 0x09, 0x00,
		0x05, 0x61, 0x00, 0xe9, 0x4d
	};

	printf("Testing cp_build_packet(CMD_ID) -- ");
	if ((len = test_cp_build_packet(p, packet, 2, 512)) < 0) {
		printf("error!\n");
		return -1;
	}
	CHECK_ARRAY(packet, len, expected);
	printf("success!\n");
	return 0;
}

int test_phy_decode_packet_ack(struct osdp * ctx)
{
	int len;
	struct osdp_pd *p = to_current_pd(ctx);
	uint8_t packet[128] = { 0xff, 0x53, 0xe5, 0x08, 0x00,
		0x05, 0x40, 0xe3, 0xa5
	};
	uint8_t expected[] = { REPLY_ACK };

	printf("Testing phy_decode_packet(REPLY_ACK) -- ");
	if ((len = phy_decode_packet(p, packet, 9)) < 0) {
		printf("error!\n");
		return -1;
	}
	CHECK_ARRAY(packet, len, expected);
	printf("success!\n");
	return 0;
}

int test_cp_queue_command(struct osdp * ctx)
{
	struct osdp_pd *p = to_current_pd(ctx);

	int len;
	uint8_t buf[128];
	uint8_t cmd96[] = {
		0x60, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x60, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x60, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	};
	uint8_t cmd32[] = {
		0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	};

	uint8_t cmd16[] = {
		0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	};

	printf("Testing cp_queue_command() -- ");

	if (cp_enqueue_command(p, (struct osdp_data *)cmd96)) {
		printf("enqueue cmd96 error!\n");
		return -1;
	}
	len = cp_dequeue_command(p, FALSE, buf, 128);
	CHECK_ARRAY(buf, len, cmd96);

	if (cp_enqueue_command(p, (struct osdp_data *)cmd32)) {
		printf("enqueue cmd32 error!\n");
		return -1;
	}
	len = cp_dequeue_command(p, FALSE, buf, 128);
	CHECK_ARRAY(buf, len, cmd32);

	if (cp_enqueue_command(p, (struct osdp_data *)cmd16)) {
		printf("enqueue cmd16 error!\n");
		return -1;
	}

	/* test readonly mode */
	len = cp_dequeue_command(p, TRUE, buf, 128);
	CHECK_ARRAY(buf, len, cmd16);

	len = cp_dequeue_command(p, FALSE, buf, 128);
	CHECK_ARRAY(buf, len, cmd16);

	printf("success!\n");
	return 0;
}

int test_cp_phy_setup(struct test *t)
{
	/* mock application data */
	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 9600,
		.init_flags = 0,
		.send_func = NULL,
		.recv_func = NULL
	};
	struct osdp *ctx = (struct osdp *) osdp_cp_setup(1, &info);
	if (ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	set_current_pd(ctx, 0);
	t->mock_data = (void *)ctx;
	return 0;
}

void test_cp_phy_teardown(struct test *t)
{
	osdp_cp_teardown(t->mock_data);
}

void run_cp_phy_tests(struct test *t)
{
	if (test_cp_phy_setup(t))
		return;

	DO_TEST(t, test_cp_build_packet_poll);
	DO_TEST(t, test_cp_build_packet_id);
	DO_TEST(t, test_phy_decode_packet_ack);
	DO_TEST(t, test_cp_queue_command);

	test_cp_phy_teardown(t);
}
