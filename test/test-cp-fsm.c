/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sun Sep 22 14:33:07 IST 2019
 */

#include <unistd.h>

#include <osdp.h>
#include "test.h"
#include "cp-private.h"

int cp_enqueue_command(osdp_t *ctx, struct cmd *c);

int test_fsm_resp = 0;

int test_cp_fsm_send(uint8_t *buf, int len)
{
    switch(buf[6]) {
    case 0x60:
        test_fsm_resp = 1;
        break;
    case 0x61:
        test_fsm_resp = 2;
        break;
    case 0x62:
        test_fsm_resp = 3;
        break;
    }
    return len;
}

int test_cp_fsm_receive(uint8_t *buf, int len)
{
    uint8_t resp_id[]  = { 0xff, 0x53, 0x65, 0x14, 0x00, 0x04, 0x45, 0xa1, 0xa2,
                           0xa3, 0xb1, 0xc1, 0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2,
                           0xe3, 0xf0, 0x29 };
    uint8_t resp_cap[] = { 0xff, 0x53, 0x65, 0x0b, 0x00, 0x05, 0x46, 0x04, 0x04,
                           0x01, 0x32, 0x11 };
    uint8_t resp_ack[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x06, 0x40, 0x60, 0xd2 };

    switch(test_fsm_resp) {
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
        .init_flags = 0,
        .send_func = test_cp_fsm_send,
        .recv_func = test_cp_fsm_receive
    };
    osdp_t *ctx = (osdp_t *)osdp_cp_setup(1, &info);
    if (ctx == NULL) {
        printf("   init failed!\n");
        return -1;
    }
    // ctx->log_level = LOG_DEBUG;
    set_current_pd(ctx, 0);
    t->mock_data = (void *)ctx;
    return 0;
}

void test_cp_fsm_teardown(struct test *t)
{
    osdp_cp_teardown(t->mock_data);
}

void run_cp_fsm_tests(struct test *t)
{
    uint32_t count=0;
    osdp_t *ctx = t->mock_data;

    printf("\nStarting CP Phy state tests\n");

    if (test_cp_fsm_setup(t))
        return;

    printf("    -- executing cp_state_update()\n");
    while (1) {
        cp_state_update(ctx);
        if (count++ > 300)
            break;
        usleep(1000);
    }
    printf("    -- cp_state_update() complete\n");

    TEST_REPORT(t, TRUE);

    test_cp_fsm_teardown(t);
}
