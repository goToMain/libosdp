/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>
#include <stddef.h>

#define OSDP_PD_ERR_RETRY_SEC			(60)
#define OSDP_PD_POLL_TIMEOUT_MS			(50)
#define OSDP_PD_CMD_QUEUE_SIZE			(128)
#define OSDP_RESP_TOUT_MS			(400)
#define OSDP_CP_RETRY_WAIT_MS			(500)
#define OSDP_PD_CAP_SENTINEL			{ -1, 0, 0 } /* struct pd_cap[] end */

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
	uint8_t output_no;
	uint8_t control_code;
	uint16_t tmr_count;
};

/* CMD_LED */
struct __osdp_cmd_led_params {
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t on_color;
	uint8_t off_color;
	uint16_t timer;
};

struct osdp_cmd_led {
	uint8_t reader;
	uint8_t number;
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

struct pd_cap {
	uint8_t function_code;	/* enum osdp_pd_cap_function_code_e */
	uint8_t compliance_level;
	uint8_t num_items;
};

struct pd_id {
	int version;
	int model;
	uint32_t vendor_code;
	uint32_t serial_number;
	uint32_t firmware_version;
};

typedef struct {
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

	/**
	 * send_func - Sends byte array into some channel
	 * @buf - byte array to be sent
	 * @len - number of bytes in `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes sent. must be <= `len` (TODO: handle partials)
	 */
	int (*send_func) (uint8_t * buf, int len);

	/**
	 * recv_func - Copies received bytes into buffer
	 * @buf - byte array copy incoming data
	 * @len - sizeof `buf`. Can copy utmost `len` number of bytes into `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes copied on to `bug`. Must be <= `len`
	 */
	int (*recv_func) (uint8_t * buf, int len);

} osdp_pd_info_t;

/**
 * The keep the OSDP internal data strucutres from polluting the exposed headers,
 * they are typecasted to void * before sending them to the upper layers. This
 * level of abstaction looked reasonable as _technically_ no one should attempt
 * to modify it outside fo the LibOSDP.
 */
typedef void *osdp_cp_t;
typedef void *osdp_pd_t;

/* --- CP Only---- */

struct osdp_cp_notifiers {
	int (*keypress) (int address, uint8_t key);
	int (*cardread) (int address, int format, uint8_t * data, int len);
};

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t * info);
void osdp_set_log_level(int log_level);
void osdp_cp_refresh(osdp_cp_t * ctx);
void osdp_cp_teardown(osdp_cp_t * ctx);

int osdp_cp_set_callback_key_press(osdp_cp_t * ctx, int (*cb) (int address, uint8_t key));
int osdp_cp_set_callback_card_read(osdp_cp_t * ctx, int (*cb) (int address, int format, uint8_t * data, int len));

int osdp_cp_send_cmd_output(osdp_cp_t * ctx, int pd, struct osdp_cmd_output *p);
int osdp_cp_send_cmd_led(osdp_cp_t * ctx, int pd, struct osdp_cmd_led *p);
int osdp_cp_send_cmd_buzzer(osdp_cp_t * ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_cp_send_cmd_comset(osdp_cp_t * ctx, int pd, struct osdp_cmd_comset *p);

/* --- PD Only --- */

osdp_pd_t *osdp_pd_setup(osdp_pd_info_t * info);
void osdp_pd_teardown(osdp_pd_t * ctx);
void osdp_pd_set_callback_cmd_led(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_led *p));
void osdp_pd_set_callback_cmd_buzzer(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_buzzer *p));
void osdp_pd_set_callback_cmd_output(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_output *p));
void osdp_pd_set_callback_cmd_text(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_text *p));
void osdp_pd_set_callback_cmd_comset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_comset *p));

#endif	/* _OSDP_H_ */
