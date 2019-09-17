/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Mon Sep 16 21:59:28 IST 2019
 */

#include "test.h"

int cp_build_packet(osdp_t *ctx, uint8_t *cmd, int clen, uint8_t *buf, int blen);
int cp_decode_packet(osdp_t *ctx, uint8_t *buf, int blen, uint8_t *cmd, int clen);
int cp_build_command(osdp_t *ctx, struct cmd *cmd, uint8_t *buf, int blen);
int cp_process_response(osdp_t *ctx, uint8_t *buf, int len);

int test_cp_build_packet_poll(osdp_t *ctx)
{
    int len;
    uint8_t packet[512];
    uint8_t cmd_buf[] = { CMD_POLL };
    uint8_t expected[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x04, 0x60, 0x60, 0x90 };

    printf("Testing cp_build_packet(CMD_POLL) -- ");
    if ((len = cp_build_packet(ctx, cmd_buf, sizeof(cmd_buf), packet, 512)) < 0) {
        printf("error!\n");
    }
    CHECK_ARRAY(packet, len, expected);
    return 0;
}

int test_cp_build_packet_id(osdp_t *ctx)
{
    int len;
    uint8_t packet[512];
    uint8_t cmd_buf[] = { CMD_ID, 0x00 };
    uint8_t expected[] = { 0xff, 0x53, 0x65, 0x09, 0x00, 0x05, 0x61, 0x00, 0xe9, 0x4d };

    printf("Testing cp_build_packet(CMD_ID) -- ");
    if ((len = cp_build_packet(ctx, cmd_buf, sizeof(cmd_buf), packet, 512)) < 0) {
        printf("error!\n");
        return -1;
    }
    CHECK_ARRAY(packet, len, expected);
    return 0;
}

int test_cp_decode_packet_ack(osdp_t *ctx)
{
    int len;
    uint8_t cmd_buf[128];
    uint8_t packet[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x05, 0x40, 0x33, 0x87 };
    uint8_t expected[] = { REPLY_ACK };

    printf("Testing cp_decode_packet(REPLY_ACK) -- ");
    if ((len = cp_decode_packet(ctx, packet, sizeof(packet), cmd_buf, 128)) < 0) {
        printf("error!\n");
        return -1;
    }
    CHECK_ARRAY(cmd_buf, len, expected);
    return 0;
}

int test_cp_build_command(osdp_t *ctx)
{
    int len;
    struct cmd c;
    struct cmd_buzzer buz;
    uint8_t cmd_buf[128];
    uint8_t expected[] = { 0x6a, 0x65, 0x00, 0x0a, 0x0a, 0x00 };

    c.id = CMD_BUZ;
    buz.on_time = 10;
    buz.off_time = 10;
    buz.reader = 101;
    buz.tone_code = 0;
    buz.rep_count = 0;
    c.arg = &buz;
    printf("Testing cp_build_command(CMD_BUZ) -- ");
    if ((len = cp_build_command(ctx, &c, cmd_buf, 128)) < 0) {
        printf("error buildinf command\n");
        return -1;
    }
    CHECK_ARRAY(cmd_buf, len, expected);
    return 0;
}

int test_cp_process_response_id(osdp_t *ctx)
{
    pd_t *p = to_current_pd(ctx);
    uint8_t resp[] = { REPLY_PDID, 0xa1, 0xa2, 0xa3, 0xb1, 0xc1,
                       0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2, 0xe3 };

    printf("Testing cp_process_response(REPLY_PDID) -- ");
    if (cp_process_response(ctx, resp, sizeof(resp)) < 0) {
        printf("error!\n");
        return -1;
    }
    if (    p->id.vendor_code != 0x00a3a2a1 ||
            p->id.model != 0xb1 ||
            p->id.version != 0xc1 ||
            p->id.serial_number != 0xd4d3d2d1 ||
            p->id.firmware_version != 0x00e1e2e3
        ) {
        printf("error data mismatch! 0x%04x 0x%02x 0x%02x 0x04%x 0x04%x\n",
               p->id.vendor_code, p->id.model, p->id.version,
               p->id.serial_number, p->id.firmware_version);
        return -1;
    }
    printf("success!");
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
    osdp_t *ctx = (osdp_t *)osdp_cp_setup(1, &info);
    if (ctx == NULL) {
        printf("   init failed!\n");
        return -1;
    }
    set_current_pd(ctx, ctx->pd);
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
    DO_TEST(t, test_cp_decode_packet_ack);
    DO_TEST(t, test_cp_build_command);
    DO_TEST(t, test_cp_process_response_id);

    test_cp_phy_teardown(t);
}
