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

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key);
void osdp_cp_refresh(osdp_cp_t *ctx);
void osdp_cp_teardown(osdp_cp_t *ctx);

/* --- Callbacks for data from PD --- */

int osdp_cp_set_callback_key_press(osdp_cp_t *ctx, int (*cb) (int address, uint8_t key));
int osdp_cp_set_callback_card_read(osdp_cp_t *ctx, int (*cb) (int address, int format, uint8_t * data, int len));

/* --- Commands to PD --- */

int osdp_cp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p);
int osdp_cp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_cp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p);
int osdp_cp_send_cmd_text(osdp_cp_t *ctx, int pd, struct osdp_cmd_text *p);
int osdp_cp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p);
int osdp_cp_send_cmd_keyset(osdp_cp_t *ctx, struct osdp_cmd_keyset *p);

/* =============================== PD Methods ===================-=========== */

/* --- State Management --- */

osdp_pd_t *osdp_pd_setup(osdp_pd_info_t * info, uint8_t *scbk);
void osdp_pd_teardown(osdp_pd_t *ctx);
void osdp_pd_refresh(osdp_pd_t *ctx);

/* --- PD application command call backs --- */

void osdp_pd_set_callback_cmd_led(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_led *p));
void osdp_pd_set_callback_cmd_buzzer(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_buzzer *p));
void osdp_pd_set_callback_cmd_output(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_output *p));
void osdp_pd_set_callback_cmd_text(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_text *p));
void osdp_pd_set_callback_cmd_comset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_comset *p));
void osdp_pd_set_callback_cmd_keyset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_keyset *p));

/* ============================= Common Methods ============================= */

/* --- Logging --- */

#define osdp_set_log_level(l) osdp_logger_init(l, NULL)
void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...));

/* --- Diagnostics --- */

const char *osdp_get_version();

#endif	/* _OSDP_H_ */
