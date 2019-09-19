/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 19:25:18 IST 2019
 */

#include "cp-private.h"

enum cp_fsm_state_e {
    CP_STATE_INIT,
    CP_STATE_IDREQ,
    CP_STATE_CAPDET,
    CP_STATE_ONLINE,
    CP_STATE_SENTINEL
};

int cp_send_wait_resp(osdp_t *ctx, int cmd_id)
{
    if (cp_enqueue_command(ctx, cmd_id, NULL, 0)) {
        osdp_log(LOG_INFO, "failed to queue command");
        return -1;
    }

    return 0;
}

/**
 * Returns:
 *  1: in-progress
 *  0: completed
 * -1: error
 */
int cp_fsm_execute(osdp_t *ctx)
{
    int ret = -1;
    cp_t *cp = to_cp(ctx);

    switch (cp->state) {
    case CP_STATE_INIT:
        break;
    case CP_STATE_ONLINE:
        break;
    case CP_STATE_IDREQ:
        break;
    case CP_STATE_CAPDET:
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}
