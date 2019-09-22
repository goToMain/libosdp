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

int cp_state_update(osdp_t *ctx)
{
    pd_t *pd = to_current_pd(ctx);
    int phy_state;

    phy_state = cp_phy_state_update(ctx);
    if (phy_state == 1) /* commands are being executed */
        return -1;

    switch (pd->state) {
    case CP_STATE_INIT:
        break;
    case CP_STATE_ONLINE:
        break;
    case CP_STATE_IDREQ:
        break;
    case CP_STATE_CAPDET:
        break;
    default:
        break;
    }

    return 0;
}
