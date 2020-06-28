/*
 * Copyright (c) 2020 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_CMD_H_
#define _OSDP_CMD_H_

/* Imported from osdp.h -- Don't include directly */

#include <stdint.h>

enum osdp_cmd_e {
	OSDP_CMD_OUTPUT = 1,
	OSDP_CMD_LED,
	OSDP_CMD_BUZZER,
	OSDP_CMD_TEXT,
	OSDP_CMD_KEYSET,
	OSDP_CMD_COMSET,
	OSDP_CMD_SENTINEL
};

/* CMD_OUT */
struct osdp_cmd_output {
	uint8_t output_no;
	uint8_t control_code;
	uint16_t tmr_count;
};

/* CMD_LED */
enum osdp_led_color_e {
	OSDP_LED_COLOR_NONE,
	OSDP_LED_COLOR_RED,
	OSDP_LED_COLOR_GREEN,
	OSDP_LED_COLOR_AMBER,
	OSDP_LED_COLOR_BLUE,
	OSDP_LED_COLOR_SENTINEL
};

struct __osdp_cmd_led_params {
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t on_color;	/* enum osdp_led_color_e */
	uint8_t off_color;	/* enum osdp_led_color_e */
	uint16_t timer;
};

struct osdp_cmd_led {
	uint8_t reader;
	uint8_t led_number;
	struct __osdp_cmd_led_params temporary;
	struct __osdp_cmd_led_params permanent;
};

/* CMD_BUZ */
struct osdp_cmd_buzzer {
	uint8_t reader;
	uint8_t tone_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t rep_count;
};

/* CMD_TEXT */
struct osdp_cmd_text {
	uint8_t reader;
	uint8_t cmd;
	uint8_t temp_time;
	uint8_t offset_row;
	uint8_t offset_col;
	uint8_t length;
	uint8_t data[32];
};

/* CMD_COMSET */
struct osdp_cmd_comset {
	uint8_t addr;
	uint32_t baud;
};

/* CMD_KEYSET */
struct osdp_cmd_keyset {
	uint8_t key_type;
	uint8_t len;
	uint8_t data[32];
};

/* generic OSDP command structure */
struct osdp_cmd {
	void *__next;
	int id;
	union {
		uint8_t cmd_bytes[32];
		struct osdp_cmd_led    led;
		struct osdp_cmd_buzzer buzzer;
		struct osdp_cmd_text   text;
		struct osdp_cmd_output output;
		struct osdp_cmd_comset comset;
		struct osdp_cmd_keyset keyset;
	};
};

#endif /* _OSDP_CMD_H_ */
