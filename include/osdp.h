/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSDP_CMD_TEXT_MAX_LEN          32

/**
 * @brief Various card formats that a PD can support. This is sent to CP
 * when a PD must report a card read.
 */
enum osdp_card_formats_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED,
	OSDP_CARD_FMT_RAW_WIEGAND,
	OSDP_CARD_FMT_ASCII,
	OSDP_CARD_FMT_SENTINEL
};

/**
 * @brief Various PD capability function codes.
 */
enum osdp_pd_cap_function_code_e {
	/**
	 * @brief Dummy.
	 */
	CAP_UNUSED,

	/**
	 * @brief This function indicates the ability to monitor the status of a
	 * switch using a two-wire electrical connection between the PD and the
	 * switch. The on/off position of the switch indicates the state of an
	 * external device.
	 *
	 * The PD may simply resolve all circuit states to an open/closed
	 * status, or it may implement supervision of the monitoring circuit.
	 * A supervised circuit is able to indicate circuit fault status in
	 * addition to open/closed status.
	 */
	CAP_CONTACT_STATUS_MONITORING,

	/**
	 * @brief This function provides a switched output, typically in the
	 * form of a relay. The Output has two states: active or inactive. The
	 * Control Panel (CP) can directly set the Output's state, or, if the PD
	 * supports timed operations, the CP can specify a time period for the
	 * activation of the Output.
	 */
	CAP_OUTPUT_CONTROL,

	/**
	 * @brief This capability indicates the form of the card data is
	 * presented to the Control Panel.
	 */
	CAP_CARD_DATA_FORMAT,

	/**
	 * @brief This capability indicates the presence of and type of LEDs.
	 */
	CAP_READER_LED_CONTROL,

	/**
	 * @brief This capability indicates the presence of and type of an
	 * Audible Annunciator (buzzer or similar tone generator)
	 */
	CAP_READER_AUDIBLE_OUTPUT,

	/**
	 * @brief This capability indicates that the PD supports a text display
	 * emulating character-based display terminals.
	 */
	CAP_READER_TEXT_OUTPUT,

	/**
	 * @brief This capability indicates that the type of date and time
	 * awareness or time keeping ability of the PD.
	 */
	CAP_TIME_KEEPING,

	/**
	 * @brief All PDs must be able to support the checksum mode. This
	 * capability indicates if the PD is capable of supporting CRC mode.
	 */
	CAP_CHECK_CHARACTER_SUPPORT,

	/**
	 * @brief This capability indicates the extent to which the PD supports
	 * communication security (Secure Channel Communication)
	 */
	CAP_COMMUNICATION_SECURITY,

	/**
	 * @brief This capability indicates the maximum size single message the
	 * PD can receive.
	 */
	CAP_RECEIVE_BUFFERSIZE,

	/**
	 * @brief This capability indicates the maximum size multi-part message
	 * which the PD can handle.
	 */
	CAP_LARGEST_COMBINED_MESSAGE_SIZE,

	/**
	 * @brief This capability indicates whether the PD supports the
	 * transparent mode used for communicating directly with a smart card.
	 */
	CAP_SMART_CARD_SUPPORT,

	/**
	 * @brief This capability indicates the number of credential reader
	 * devices present. Compliance levels are bit fields to be assigned as
	 * needed.
	 */
	CAP_READERS,

	/**
	 * @brief This capability indicates the ability of the reader to handle
	 * biometric input
	 */
	CAP_BIOMETRICS,

	/**
	 * @brief Capability Sentinel
	 */
	CAP_SENTINEL
};

/**
 * @brief PD capability structure. Each PD capability has a 3 byte
 * representation.
 *
 * @param function_code One of enum osdp_pd_cap_function_code_e.
 * @param compliance_level A function_code dependent number that indicates what
 *                         the PD can do with this capability.
 * @param num_items Number of such capability entities in PD.
 */
struct pd_cap {
	uint8_t function_code;
	uint8_t compliance_level;
	uint8_t num_items;
};

/**
 * @brief PD ID information advertised by the PD.
 *
 * @param version 3-bytes IEEE assigned OUI
 * @param model 1-byte Manufacturer's model number
 * @param vendor_code 1-Byte Manufacturer's version number
 * @param serial_number 4-byte serial number for the PD
 * @param firmware_version 3-byte version (major, minor, build)
 */
struct pd_id {
	int version;
	int model;
	uint32_t vendor_code;
	uint32_t serial_number;
	uint32_t firmware_version;
};

struct osdp_channel {
	/**
	 * @brief pointer to a block of memory that will be passed to the
	 * send/receive method. This is optional and can be left empty.
	 */
	void *data;

	/**
	 * @brief pointer to function that copies received bytes into buffer
	 * @param data for use by underlying layers. channel_s::data is passed
	 * @param buf byte array copy incoming data
	 * @param len sizeof `buf`. Can copy utmost `len` bytes into `buf`
	 *
	 * @retval +ve: number of bytes copied on to `bug`. Must be <= `len`
	 * @retval -ve on errors
	 */
	int (*recv)(void *data, uint8_t *buf, int maxlen);

	/**
	 * @brief pointer to function that sends byte array into some channel
	 * @param data for use by underlying layers. channel_s::data is passed
	 * @param buf byte array to be sent
	 * @param len number of bytes in `buf`
	 *
	 * @retval +ve: number of bytes sent. must be <= `len`
	 * @retval -ve on errors
	 */
	int (*send)(void *data, uint8_t *buf, int len);

	/**
	 * @brief pointer to function that drops all bytes in TX/RX fifo
	 * @param data for use by underlying layers. channel_s::data is passed
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
	 * capabilities. Use { -1, 0, 0 } to terminate the array. This is used
	 * only PD mode of operation.
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
 * they are typedefed to void before sending them to the upper layers. This
 * level of abstaction looked reasonable as _technically_ no one should attempt
 * to modify it outside fo the LibOSDP and their definition may change at any
 * time.
 */
typedef void osdp_t;

/**
 * @brief Command sent from CP to Control digital output of PD.
 *
 * @param output_no 0 = First Output, 1 = Second Output, etc.
 * @param control_code One of the following:
 *   0 - NOP – do not alter this output
 *   1 - set the permanent state to OFF, abort timed operation (if any)
 *   2 - set the permanent state to ON, abort timed operation (if any)
 *   3 - set the permanent state to OFF, allow timed operation to complete
 *   4 - set the permanent state to ON, allow timed operation to complete
 *   5 - set the temporary state to ON, resume perm state on timeout
 *   6 - set the temporary state to OFF, resume permanent state on timeout
 * @param tmr_count Time in units of 100 ms
 */
struct osdp_cmd_output {
	uint8_t output_no;
	uint8_t control_code;
	uint16_t tmr_count;
};

/**
 * @brief LED Colors as specified in OSDP for the on_color/off_color parameters.
 */
enum osdp_led_color_e {
	OSDP_LED_COLOR_NONE,
	OSDP_LED_COLOR_RED,
	OSDP_LED_COLOR_GREEN,
	OSDP_LED_COLOR_AMBER,
	OSDP_LED_COLOR_BLUE,
	OSDP_LED_COLOR_SENTINEL
};

/**
 * @brief LED params sub-structure. Part of LED command. See struct osdp_cmd_led
 *
 * @param control_code One of the following:
 * Temporary Control Code:
 *   0 - NOP - do not alter this LED's temporary settings
 *   1 - Cancel any temporary operation and display this LED's permanent state
 *       immediately
 *   2 - Set the temporary state as given and start timer immediately
 * Permanent Control Code:
 *   0 - NOP - do not alter this LED's permanent settings
 *   1 - Set the permanent state as given
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param on_color Color to set during the ON timer (enum osdp_led_color_e)
 * @param off_color Color to set during the OFF timer (enum osdp_led_color_e)
 * @param timer Time in units of 100 ms (only for temporary mode)
 */
struct __osdp_cmd_led_params {
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t on_color;
	uint8_t off_color;
	uint16_t timer;
};

/**
 * @brief Sent from CP to PD to control the behaviour of it's on-board LEDs
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param led_number 0 = first LED, 1 = second LED, etc.
 * @param temporary ephemeral LED status descriptor
 * @param permanent permanent LED status descriptor
 */
struct osdp_cmd_led {
	uint8_t reader;
	uint8_t led_number;
	struct __osdp_cmd_led_params temporary;
	struct __osdp_cmd_led_params permanent;
};

/**
 * @brief Sent from CP to control the behaviour of a buzzer in the PD.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param tone_code 0: no tone, 1: off, 2: default tone, 3+ is TBD.
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param rep_count The number of times to repeat the ON/OFF cycle; 0: forever
 */
struct osdp_cmd_buzzer {
	uint8_t reader;
	uint8_t tone_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t rep_count;
};

/**
 * @brief Command to manuplate any display units that the PD supports.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param cmd  One of the following:
 *   1 - permanent text, no wrap
 *   2 - permanent text, with wrap
 *   3 - temp text, no wrap
 *   4 - temp text, with wrap
 * @param temp_time duration to display temporary text, in seconds
 * @param offset_row row to display the first character (1 indexed)
 * @param offset_col column to display the first character (1 indexed)
 * @param length Number of characters in the string
 * @param data The string to display
 */
struct osdp_cmd_text {
	uint8_t reader;
	uint8_t cmd;
	uint8_t temp_time;
	uint8_t offset_row;
	uint8_t offset_col;
	uint8_t length;
	uint8_t data[OSDP_CMD_TEXT_MAX_LEN];
};

/**
 * @brief Sent in response to a COMSET command. Set communication parameters to
 * PD. Must be stored in PD non-volatile memory.
 *
 * @param addr Unit ID to which this PD will respond after the change takes
 *             effect.
 * @param baud baud rate value 9600/38400/115200
 */
struct osdp_cmd_comset {
	uint8_t addr;
	uint32_t baud;
};

/**
 * @brief This command transfers an encryption key from the CP to a PD.
 *
 * @param key_type Type of keys:
 *                  - 0x01 – Secure Channel Base Key
 * @param len Number of bytes of key data - (Key Length in bits + 7) / 8
 * @param data Key data
 */
struct osdp_cmd_keyset {
	uint8_t key_type;
	uint8_t len;
	uint8_t data[32];
};

/**
 * @brief OSDP application exposed commands
 */
enum osdp_cmd_e {
	OSDP_CMD_OUTPUT = 1,
	OSDP_CMD_LED,
	OSDP_CMD_BUZZER,
	OSDP_CMD_TEXT,
	OSDP_CMD_KEYSET,
	OSDP_CMD_COMSET,
	OSDP_CMD_SENTINEL
};

/**
 * @brief OSDP Command Structure. This is a wrapper for all individual OSDP
 * commands.
 *
 * @param __next INTERNAL. Don't use.
 * @param id used to select specific commands in union. Type: enum osdp_cmd_e
 * @param cmd_bytes INTERNAL. Don't use.
 * @param led LED command structure
 * @param buzzer buzzer command structure
 * @param text text command structure
 * @param output output command structure
 * @param comset comset command structure
 * @param keyset keyset command structure
 */
struct osdp_cmd {
	void *__next;
	int id;
	union {
		struct osdp_cmd_led    led;
		struct osdp_cmd_buzzer buzzer;
		struct osdp_cmd_text   text;
		struct osdp_cmd_output output;
		struct osdp_cmd_comset comset;
		struct osdp_cmd_keyset keyset;
	};
};

/* =============================== CP Methods =============================== */

osdp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key);
void osdp_cp_refresh(osdp_t *ctx);
void osdp_cp_teardown(osdp_t *ctx);

int osdp_cp_set_callback_key_press(osdp_t *ctx, int (*cb) (int address, uint8_t key));
int osdp_cp_set_callback_card_read(osdp_t *ctx, int (*cb) (int address, int format, uint8_t * data, int len));

int osdp_cp_send_cmd_led(osdp_t *ctx, int pd, struct osdp_cmd_led *p);
int osdp_cp_send_cmd_buzzer(osdp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_cp_send_cmd_output(osdp_t *ctx, int pd, struct osdp_cmd_output *p);
int osdp_cp_send_cmd_text(osdp_t *ctx, int pd, struct osdp_cmd_text *p);
int osdp_cp_send_cmd_comset(osdp_t *ctx, int pd, struct osdp_cmd_comset *p);
int osdp_cp_send_cmd_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p);

/* =============================== PD Methods ===================-=========== */

osdp_t *osdp_pd_setup(osdp_pd_info_t * info, uint8_t *scbk);
void osdp_pd_teardown(osdp_t *ctx);
void osdp_pd_refresh(osdp_t *ctx);

/**
 * @param ctx pointer to osdp context
 * @param cmd pointer to a command structure that would be filled by the driver.
 *
 * @retval 0 on success.
 * @retval -1 on failure.
 */
int osdp_pd_get_cmd(osdp_t *ctx, struct osdp_cmd *cmd);

/* ============================= Common Methods ============================= */

#define osdp_set_log_level(l) osdp_logger_init(l, NULL)
void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...));
const char *osdp_get_version();
const char *osdp_get_source_info();

uint32_t osdp_get_status_mask(osdp_t *ctx);
uint32_t osdp_get_sc_status_mask(osdp_t *ctx);

#ifdef __cplusplus
}
#endif

#endif	/* _OSDP_H_ */
