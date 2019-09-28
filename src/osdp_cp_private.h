/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 19:25:18 IST 2019
 */

#ifndef _CP_PRIVATE_H_
#define _CP_PRIVATE_H_

#include "osdp_common.h"

enum {
	CP_PHY_STATE_IDLE,
	CP_PHY_STATE_SEND_CMD,
	CP_PHY_STATE_RESP_WAIT,
	CP_PHY_STATE_ERR,
};

enum cp_fsm_state_e {
	CP_STATE_INIT,
	CP_STATE_IDREQ,
	CP_STATE_CAPDET,
	CP_STATE_ONLINE,
	CP_STATE_OFFLINE,
	CP_STATE_SENTINEL
};

int cp_phy_state_update(pd_t * pd);
void phy_state_reset(pd_t * pd);
int cp_state_update(pd_t * pd);
int cp_enqueue_command(pd_t * pd, struct cmd *c);

#endif	/* _CP_PRIVATE_H_ */
