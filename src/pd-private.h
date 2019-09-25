/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Tue Sep 24 19:24:49 IST 2019
 */


#ifndef _PD_PRIVATE_H_
#define _PD_PRIVATE_H_

#include "common.h"

enum {
    PD_PHY_STATE_IDLE,
    PD_PHY_STATE_SEND_REPLY,
    PD_PHY_STATE_ERR,
};

int pd_phy_state_update(pd_t *pd);

#endif /* _PD_PRIVATE_H_ */
