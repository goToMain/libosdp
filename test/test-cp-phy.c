/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Mon Sep 16 21:59:28 IST 2019
 */

#include "test.h"

int cp_build_packet(osdp_t *ctx, uint8_t *cmd, int clen, uint8_t *buf, int blen);

int test_cp_build_packet_poll(osdp_t *ctx)
{
    int len;
    uint8_t packet[512];
    uint8_t cmd_buf[] = { CMD_POLL };
    uint8_t expected_result[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x04, 0x60, 0x60, 0x90 };

    printf("Testing cp_build_packet(CMD_POLL) -- ");
    len = cp_build_packet(ctx, cmd_buf, sizeof(cmd_buf), packet, 512);

    if ((len != sizeof(expected_result)) || memcmp(packet, expected_result, sizeof(expected_result))) {
        printf("error!\n");
        hexdump("  Expected: ", expected_result, sizeof(expected_result));
        hexdump("  Got", packet, len);
        return -1;
    }

    printf("success!\n");
    return 0;
}

int test_cp_build_packet_id(osdp_t *ctx)
{
    int len;
    uint8_t packet[512];
    uint8_t cmd_buf[] = { CMD_ID, 0x00 };
    uint8_t expected_result[] = { 0xff, 0x53, 0x65, 0x09, 0x00, 0x05, 0x61, 0x00, 0xe9, 0x4d };

    printf("Testing cp_build_packet(CMD_ID) -- ");
    len = cp_build_packet(ctx, cmd_buf, sizeof(cmd_buf), packet, 512);

    if ((len != sizeof(expected_result)) || memcmp(packet, expected_result, sizeof(expected_result))) {
        printf("error!\n");
        hexdump("  Expected: ", expected_result, sizeof(expected_result));
        hexdump("  Got", packet, len);
        return -1;
    }

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

    test_cp_phy_teardown(t);
}
