/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>
#include <stddef.h>

#include "osdp_cmd.h"
#include "osdp_types.h"
#include "osdp_config.h"

/* =============================== CP Methods =============================== */

/* --- State Management --- */

osdp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key);
void osdp_cp_refresh(osdp_t *ctx);
void osdp_cp_teardown(osdp_t *ctx);

/* --- Callbacks for data from PD --- */

int osdp_cp_set_callback_key_press(osdp_t *ctx, int (*cb) (int address, uint8_t key));
int osdp_cp_set_callback_card_read(osdp_t *ctx, int (*cb) (int address, int format, uint8_t * data, int len));

/* --- Commands to PD --- */

int osdp_cp_send_cmd_led(osdp_t *ctx, int pd, struct osdp_cmd_led *p);
int osdp_cp_send_cmd_buzzer(osdp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_cp_send_cmd_output(osdp_t *ctx, int pd, struct osdp_cmd_output *p);
int osdp_cp_send_cmd_text(osdp_t *ctx, int pd, struct osdp_cmd_text *p);
int osdp_cp_send_cmd_comset(osdp_t *ctx, int pd, struct osdp_cmd_comset *p);
int osdp_cp_send_cmd_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p);

/* =============================== PD Methods ===================-=========== */

/* --- State Management --- */

osdp_t *osdp_pd_setup(osdp_pd_info_t * info, uint8_t *scbk);
void osdp_pd_teardown(osdp_t *ctx);
void osdp_pd_refresh(osdp_t *ctx);
int osdp_pd_get_cmd(osdp_t *ctx, struct osdp_cmd *cmd);

/* ============================= Common Methods ============================= */

/* --- Logging / Diagnostics --- */

#define osdp_set_log_level(l) osdp_logger_init(l, NULL)
void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...));
const char *osdp_get_version();
const char *osdp_get_source_info();

/* --- Status --- */

uint32_t osdp_get_status_mask(osdp_t *ctx);
uint32_t osdp_get_sc_status_mask(osdp_t *ctx);

#endif	/* _OSDP_H_ */
