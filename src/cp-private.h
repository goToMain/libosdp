/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 19:25:18 IST 2019
 */

#ifndef _CP_PRIVATE_H_
#define _CP_PRIVATE_H_

#include "common.h"

int cp_phy_state_update(osdp_t *ctx);
void cp_phy_state_reset(pd_t *pd);
int cp_state_update(osdp_t *ctx);
int cp_enqueue_command(osdp_t *ctx, struct cmd *c);

#endif /* _CP_PRIVATE_H_ */
