/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <kernel.h>

#define OSDP_PD_ERR_RETRY_SEC			(60)
#define OSDP_PD_POLL_TIMEOUT_MS		   	(50)
#define OSDP_PD_CMD_QUEUE_SIZE			(128)
#define OSDP_RESP_TOUT_MS			(400)
#define OSDP_PD_RX_BUF_LENGTH			(512)
#define OSDP_PD_CAP_SENTINEL			{ -1, 0, 0 }	/* struct pd_cap[] end */

enum osdp_card_formats_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED,
	OSDP_CARD_FMT_RAW_WIEGAND,
	OSDP_CARD_FMT_ASCII,
	OSDP_CARD_FMT_SENTINEL
};

/* struct pd_cap::function_code */
enum osdp_pd_cap_function_code_e {
	CAP_UNUSED,
	CAP_CONTACT_STATUS_MONITORING,
	CAP_OUTPUT_CONTROL,
	CAP_CARD_DATA_FORMAT,
	CAP_READER_LED_CONTROL,
	CAP_READER_AUDIBLE_OUTPUT,
	CAP_READER_TEXT_OUTPUT,
	CAP_TIME_KEEPING,
	CAP_CHECK_CHARACTER_SUPPORT,
	CAP_COMMUNICATION_SECURITY,
	CAP_RECEIVE_BUFFERSIZE,
	CAP_LARGEST_COMBINED_MESSAGE_SIZE,
	CAP_SMART_CARD_SUPPORT,
	CAP_READERS,
	CAP_BIOMETRICS,
	CAP_SENTINEL
};

/* CMD_OUT */
struct osdp_cmd_output {
	u8_t output_no;
	u8_t control_code;
	u16_t tmr_count;
};

/* CMD_LED */
struct __osdp_cmd_led_params {
	u8_t control_code;
	u8_t on_count;
	u8_t off_count;
	u8_t on_color;
	u8_t off_color;
	u16_t timer;
};

struct osdp_cmd_led {
	u8_t reader;
	u8_t number;
	struct __osdp_cmd_led_params temporary;
	struct __osdp_cmd_led_params permanent;
};

/* CMD_BUZ */
struct osdp_cmd_buzzer {
	u8_t reader;
	u8_t tone_code;
	u8_t on_count;
	u8_t off_count;
	u8_t rep_count;
};

/* CMD_TEXT */
struct osdp_cmd_text {
	u8_t reader;
	u8_t cmd;
	u8_t temp_time;
	u8_t offset_row;
	u8_t offset_col;
	u8_t length;
	u8_t data[32];
};

/* CMD_COMSET */
struct osdp_cmd_comset {
	u8_t addr;
	u32_t baud;
};

struct pd_cap {
	u8_t function_code;	/* enum osdp_pd_cap_function_code_e */
	u8_t compliance_level;
	u8_t num_items;
};

struct pd_id {
	int version;
	int model;
	u32_t vendor_code;
	u32_t serial_number;
	u32_t firmware_version;
};

struct osdp_pd_info {

	/**
	 * Can be one of 9600/38400/115200.
	 */
	int baud_rate;

	/**
	 * 7 bit PD address. the rest of the bits are ignored. The special address
	 * 0x7F is used for broadcast. So there can be 2^7-1 devices on a multi-
	 * drop channel.
	 */
	int address;

	/**
	 * Used to modify the way the context is setup.
	 */
	int init_flags;

	/**
	 * Static info that the PD reports to the CP when it received a `CMD_ID`.
	 * This is used only in PD mode of operation.
	 */
	struct pd_id id;

	/**
	 * This is a pointer to an array of structures containing the PD's
	 * capabilities. Use macro `OSDP_PD_CAP_SENTINEL` to terminate the array.
	 * This is used only PD mode of operation.
	 */
	struct pd_cap *cap;
};

int osdp_pd_setup(struct osdp_pd_info *p);
void osdp_pd_refresh();
void osdp_pd_set_callback_cmd_led(int (*cb) (struct osdp_cmd_led *p));
void osdp_pd_set_callback_cmd_buzzer(int (*cb) (struct osdp_cmd_buzzer *p));
void osdp_pd_set_callback_cmd_output(int (*cb) (struct osdp_cmd_output *p));
void osdp_pd_set_callback_cmd_text(int (*cb) (struct osdp_cmd_text *p));
void osdp_pd_set_callback_cmd_comset(int (*cb) (struct osdp_cmd_comset *p));

#endif	/* _OSDP_H_ */
