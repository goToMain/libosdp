/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 19:25:18 IST 2019
 */

#ifndef _CP_PRIVATE_H_
#define _CP_PRIVATE_H_

#include "common.h"

int cp_fsm_execute(osdp_t *ctx);
int cp_enqueue_command(osdp_t *ctx, int cmd_id, uint8_t *data, int len);
int cp_dequeue_command(osdp_t *ctx, int readonly, uint8_t *cmd_buf, int maxlen);

#endif /* _CP_PRIVATE_H_ */
