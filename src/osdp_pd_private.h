/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _PD_PRIVATE_H_
#define _PD_PRIVATE_H_

#include "osdp_common.h"

enum {
	PD_PHY_STATE_IDLE,
	PD_PHY_STATE_SEND_REPLY,
	PD_PHY_STATE_ERR,
};

int pd_phy_state_update(pd_t * pd);

#endif	/* _PD_PRIVATE_H_ */
