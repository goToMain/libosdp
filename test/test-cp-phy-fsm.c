/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Fri Sep 20 19:21:16 IST 2019
 */

#include <osdp.h>
#include "test.h"
#include "cp-private.h"

int cp_enqueue_command(osdp_t *ctx, struct cmd *c);

int phy_fsm__resp_offset = 0;

int test_cp_phy_fsm_send(uint8_t *buf, int len)
{
    const uint8_t phy_fsm__cmd_poll[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x04,
                                          0x60, 0x60, 0x90 };
    const uint8_t phy_fsm__cmd_id[]   = { 0xff, 0x53, 0x65, 0x09, 0x00, 0x05,
                                          0x61, 0x00, 0xe9, 0x4d };
    switch(phy_fsm__resp_offset) {
    case 0:
        if (memcmp(buf, phy_fsm__cmd_poll, len) != 0) {
            printf("    -- poll buf Mismatch!\n");
            osdp_dump("Attempt to send", buf, len);
        }
        break;
    case 1:
        if (memcmp(buf, phy_fsm__cmd_id, len) != 0) {
            printf("    -- id buf Mismatch!\n");
            osdp_dump("Attempt to send", buf, len);
        }
        break;
    }
    return 0;
}

int test_cp_phy_fsm_receive(uint8_t *buf, int len)
{
    const uint8_t phy_fsm__resp_ack[] = { 0xff, 0x53, 0x65, 0x08, 0x00, 0x04,
                                          0x40, 0x02, 0xb4 };
    const uint8_t phy_fsm__resp_id[]  = { 0xff, 0x53, 0x65, 0x14, 0x00, 0x05,
                                          0x45, 0xa1, 0xa2, 0xa3, 0xb1, 0xc1,
                                          0xd1, 0xd2, 0xd3, 0xd4, 0xe1, 0xe2,
                                          0xe3, 0x91, 0x52 };
    switch(phy_fsm__resp_offset) {
    case 0:
        memcpy(buf, phy_fsm__resp_ack, sizeof(phy_fsm__resp_ack));
        phy_fsm__resp_offset++;
        return sizeof(phy_fsm__resp_ack);
    case 1:
        memcpy(buf, phy_fsm__resp_id, sizeof(phy_fsm__resp_id));
        phy_fsm__resp_offset++;
        return sizeof(phy_fsm__resp_id);
    }
    return 0;
}

int test_cp_phy_fsm_setup(struct test *t)
{
    /* mock application data */
    osdp_pd_info_t info = {
        .address = 101,
        .baud_rate = 9600,
        .init_flags = 0,
        .send_func = test_cp_phy_fsm_send,
        .recv_func = test_cp_phy_fsm_receive
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

void test_cp_phy_fsm_teardown(struct test *t)
{
    osdp_cp_teardown(t->mock_data);
}

void run_cp_phy_fsm_tests(struct test *t)
{
    int ret;
    osdp_t *ctx = t->mock_data;
    pd_t *p;

    printf("\nStarting CP Phy state tests\n");

    struct cmd cmd_poll = { .id = CMD_POLL, .len = 2 };
    struct cmd cmd_id = { .id = CMD_ID, .len = 2 };

    if (test_cp_phy_fsm_setup(t))
        return;

    p = to_current_pd(ctx);

    if (cp_enqueue_command(ctx, &cmd_poll)) {
        printf("enqueue cmd_poll error!\n");
        return;
    }

    if (cp_enqueue_command(ctx, &cmd_id)) {
        printf("enqueue cmd_poll error!\n");
        return;
    }

    printf("    -- executing test_cp_phy_fsm()\n");
    while (1) {
        ret = cp_phy_state_update(ctx);
        if (ret != 1 && ret != 2)
            break;
        /* continue when in command and between commands */
    }

    if (    p->id.vendor_code != 0x00a3a2a1 ||
            p->id.model != 0xb1 ||
            p->id.version != 0xc1 ||
            p->id.serial_number != 0xd4d3d2d1 ||
            p->id.firmware_version != 0x00e1e2e3
        ) {
        printf("    -- error ID mismatch! 0x%04x 0x%02x 0x%02x 0x04%x 0x04%x\n",
               p->id.vendor_code, p->id.model, p->id.version,
               p->id.serial_number, p->id.firmware_version);
        return;
    }

    printf("    -- test_cp_phy_fsm() complete -- %d\n", ret);

    test_cp_phy_fsm_teardown(t);
}
