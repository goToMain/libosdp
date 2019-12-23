/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>
#include <stddef.h>

#define OSDP_PD_ERR_RETRY_SEC			(300 * 1000)
#define OSDP_PD_POLL_TIMEOUT_MS			(50)
#define OSDP_RESP_TOUT_MS			(400)
#define OSDP_CP_RETRY_WAIT_MS			(500)

#define OSDP_PD_CMD_QUEUE_SIZE			(128)
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
union osdp_cmd {
	struct osdp_cmd_led led;
	struct osdp_cmd_buzzer buzzer;
	struct osdp_cmd_text text;
	struct osdp_cmd_output output;
	struct osdp_cmd_comset comset;
	struct osdp_cmd_keyset keyset;
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

struct osdp_channel {
	/**
	 * pointer to a block of memory that will be passed to the send/receive
	 * method. This is optional and can be left empty.
	 */
        void *data;

	/**
	 * recv: function pointer; Copies received bytes into buffer
	 * @data - for use by underlying layers. channel_s::data is passed
	 * @buf  - byte array copy incoming data
	 * @len  - sizeof `buf`. Can copy utmost `len` number of bytes into `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes copied on to `bug`. Must be <= `len`
	 */
	int (*recv)(void *data, uint8_t *buf, int maxlen);

	/**
	 * send: function pointer; Sends byte array into some channel
	 * @data - for use by underlying layers. channel_s::data is passed
	 * @buf  - byte array to be sent
	 * @len  - number of bytes in `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes sent. must be <= `len` (TODO: handle partials)
	 */
	int (*send)(void *data, uint8_t *buf, int len);

	/**
	 * flush: function pointer; drop all bytes in queue to be read
	 * @data - for use by underlying layers. channel_s::data is passed
	 */
	void (*flush)(void *data);
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
	int flags;

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
	 * Communication channel ops structure, containing send/recv function
	 * pointers.
	 */
	struct osdp_channel channel;
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

osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key);
void osdp_set_log_level(int log_level);
void osdp_cp_refresh(osdp_cp_t *ctx);
void osdp_cp_teardown(osdp_cp_t *ctx);

int osdp_cp_set_callback_key_press(osdp_cp_t *ctx, int (*cb) (int address, uint8_t key));
int osdp_cp_set_callback_card_read(osdp_cp_t *ctx, int (*cb) (int address, int format, uint8_t * data, int len));

int osdp_cp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p);
int osdp_cp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_cp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p);
int osdp_cp_send_cmd_text(osdp_cp_t *ctx, int pd, struct osdp_cmd_text *p);
int osdp_cp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p);
int osdp_cp_send_cmd_keyset(osdp_cp_t *ctx, struct osdp_cmd_keyset *p);

/* --- PD Only --- */

osdp_pd_t *osdp_pd_setup(osdp_pd_info_t * info, uint8_t *scbk);
void osdp_pd_teardown(osdp_pd_t *ctx);
void osdp_pd_refresh(osdp_pd_t *ctx);
void osdp_pd_set_callback_cmd_led(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_led *p));
void osdp_pd_set_callback_cmd_buzzer(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_buzzer *p));
void osdp_pd_set_callback_cmd_output(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_output *p));
void osdp_pd_set_callback_cmd_text(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_text *p));
void osdp_pd_set_callback_cmd_comset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_comset *p));
void osdp_pd_set_callback_cmd_keyset(osdp_pd_t *ctx, int (*cb) (struct osdp_cmd_keyset *p));

/* --- Diagnostics --- */

const char *osdp_get_version();
const char *osdp_get_git_tag();
const char *osdp_get_git_rev();
const char *osdp_get_git_branch();

#endif	/* _OSDP_H_ */
