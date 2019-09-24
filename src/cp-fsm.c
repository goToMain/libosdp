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
    CP_STATE_OFFLINE,
    CP_STATE_SENTINEL
};

#define cp_set_offline(p)                               \
    do {                                                \
        p->state = CP_STATE_OFFLINE;                    \
        p->tstamp = millis_now();                       \
    } while (0)

#define cp_set_state(p, s)                              \
    do {                                                \
        p->state = s;                                   \
        clear_flag(p, PD_FLAG_AWAIT_RESP);              \
    } while (0)

#define cp_reset_state(p)                               \
    do {                                                \
        p->state = CP_STATE_INIT;                       \
        phy_state_reset(p);                          \
        p->flags = 0;                                   \
    } while (0)

int cp_cmd_dispatcher(pd_t *p, int cmd)
{
    struct cmd c;

    if (isset_flag(p, PD_FLAG_AWAIT_RESP)) {
        clear_flag(p, PD_FLAG_AWAIT_RESP);
        return 0;
    }

    c.id = cmd;
    c.len = sizeof(struct cmd);
    if (cp_enqueue_command(p, &c) != 0) {
        osdp_log(LOG_WARNING, "command_enqueue error!");
    }
    set_flag(p, PD_FLAG_AWAIT_RESP);
    return 1;
}

int cp_state_update(pd_t *pd)
{
    int phy_state;

    phy_state = cp_phy_state_update(pd);
    if (phy_state == 1 ||           /* commands are being executed */
            phy_state == 2)         /* in-between commands */
        return -1 * phy_state;

    /* phy state error -- cleanup */
    if (phy_state < 0) {
        cp_set_offline(pd);
    }

    /* command queue is empty and last command was successfull */

    switch (pd->state) {
    case CP_STATE_ONLINE:
        if (millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS)
            break;
        if (cp_cmd_dispatcher(pd, CMD_POLL) == 0)
            pd->tstamp = millis_now();
        break;
    case CP_STATE_OFFLINE:
        if (millis_since(pd->tstamp) > OSDP_PD_ERR_RETRY_SEC)
            cp_reset_state(pd);
        break;
    case CP_STATE_INIT:
        cp_set_state(pd, CP_STATE_IDREQ);
        /* no break */
    case CP_STATE_IDREQ:
        if (cp_cmd_dispatcher(pd, CMD_ID) != 0)
            break;
        cp_set_state(pd, CP_STATE_CAPDET);
        /* no break */
    case CP_STATE_CAPDET:
        if (cp_cmd_dispatcher(pd, CMD_CAP) != 0)
            break;
        cp_set_state(pd, CP_STATE_ONLINE);
        break;
    default:
        break;
    }

    return 0;
}
