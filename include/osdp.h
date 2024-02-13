/*
 * Copyright (c) 2019-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OSDP setup flags. See osdp_pd_info_t::flags
 */

/**
 * @brief ENFORCE_SECURE: Make security conscious assumptions (see below) where
 * possible. Fail where these assumptions don't hold.
 *   - Don't allow use of SCBK-D.
 *   - Assume that a KEYSET was successful at an earlier time.
 *
 * @note This flag is recommended in production use.
 */
#define OSDP_FLAG_ENFORCE_SECURE 0x00010000

/**
 * @brief When set, the PD would allow one session of secure channel to be
 * setup with SCBK-D.
 *
 * @note In this mode, the PD is in a vulnerable state, the application is
 * responsible for making sure that the device enters this mode only during
 * controlled/provisioning-time environments.
 */
#define OSDP_FLAG_INSTALL_MODE 0x00020000

/**
 * @brief When set, CP will not error and fail when the PD sends an unknown,
 * unsolicited response (in response to osdp_POLL command).
 *
 * @note In PD mode this flag has no use.
 */
#define OSDP_FLAG_IGN_UNSOLICITED 0x00040000

/**
 * @brief Various PD capability function codes.
 */
enum osdp_pd_cap_function_code_e {
	/**
	 * @brief Dummy.
	 */
	OSDP_PD_CAP_UNUSED,

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
	OSDP_PD_CAP_CONTACT_STATUS_MONITORING,

	/**
	 * @brief This function provides a switched output, typically in the
	 * form of a relay. The Output has two states: active or inactive. The
	 * Control Panel (CP) can directly set the Output's state, or, if the PD
	 * supports timed operations, the CP can specify a time period for the
	 * activation of the Output.
	 */
	OSDP_PD_CAP_OUTPUT_CONTROL,

	/**
	 * @brief This capability indicates the form of the card data is
	 * presented to the Control Panel.
	 */
	OSDP_PD_CAP_CARD_DATA_FORMAT,

	/**
	 * @brief This capability indicates the presence of and type of LEDs.
	 */
	OSDP_PD_CAP_READER_LED_CONTROL,

	/**
	 * @brief This capability indicates the presence of and type of an
	 * Audible Annunciator (buzzer or similar tone generator)
	 */
	OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,

	/**
	 * @brief This capability indicates that the PD supports a text display
	 * emulating character-based display terminals.
	 */
	OSDP_PD_CAP_READER_TEXT_OUTPUT,

	/**
	 * @brief This capability indicates that the type of date and time
	 * awareness or time keeping ability of the PD.
	 */
	OSDP_PD_CAP_TIME_KEEPING,

	/**
	 * @brief All PDs must be able to support the checksum mode. This
	 * capability indicates if the PD is capable of supporting CRC mode.
	 */
	OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT,

	/**
	 * @brief This capability indicates the extent to which the PD supports
	 * communication security (Secure Channel Communication)
	 */
	OSDP_PD_CAP_COMMUNICATION_SECURITY,

	/**
	 * @brief This capability indicates the maximum size single message the
	 * PD can receive.
	 */
	OSDP_PD_CAP_RECEIVE_BUFFERSIZE,

	/**
	 * @brief This capability indicates the maximum size multi-part message
	 * which the PD can handle.
	 */
	OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE,

	/**
	 * @brief This capability indicates whether the PD supports the
	 * transparent mode used for communicating directly with a smart card.
	 */
	OSDP_PD_CAP_SMART_CARD_SUPPORT,

	/**
	 * @brief This capability indicates the number of credential reader
	 * devices present. Compliance levels are bit fields to be assigned as
	 * needed.
	 */
	OSDP_PD_CAP_READERS,

	/**
	 * @brief This capability indicates the ability of the reader to handle
	 * biometric input
	 */
	OSDP_PD_CAP_BIOMETRICS,

	/**
	 * @brief Capability Sentinel
	 */
	OSDP_PD_CAP_SENTINEL
};

enum osdp_pd_nak_code_e {
	/**
	 * @brief Dummy
	 */
	OSDP_PD_NAK_NONE,
	/**
	 * @brief Message check character(s) error (bad cksum/crc)
	 */
	OSDP_PD_NAK_MSG_CHK,
	/**
	 * @brief Command length error
	 */
	OSDP_PD_NAK_CMD_LEN,
	/**
	 * @brief Unknown Command Code – Command not implemented by PD
	 */
	OSDP_PD_NAK_CMD_UNKNOWN,
	/**
	 * @brief Sequence number error
	 */
	OSDP_PD_NAK_SEQ_NUM,
	/**
	 * @brief Secure Channel is not supported by PD
	 */
	OSDP_PD_NAK_SC_UNSUP,
	/**
	 * @brief unsupported security block or security conditions not met
	 */
	OSDP_PD_NAK_SC_COND,
	/**
	 * @brief BIO_TYPE not supported
	 */
	OSDP_PD_NAK_BIO_TYPE,
	/**
	 * @brief BIO_FORMAT not supported
	 */
	OSDP_PD_NAK_BIO_FMT,
	/**
	 * @brief Unable to process command record
	 */
	OSDP_PD_NAK_RECORD,
	/**
	 * @brief NAK codes max value
	 */
	OSDP_PD_NAK_SENTINEL
};

/**
 * @brief PD capability structure. Each PD capability has a 3 byte
 * representation.
 *
 * @param function_code One of enum osdp_pd_cap_function_code_e.
 * @param compliance_level A function_code dependent number that indicates what
 *        the PD can do with this capability.
 * @param num_items Number of such capability entities in PD.
 */
struct osdp_pd_cap {
	uint8_t function_code;
	uint8_t compliance_level;
	uint8_t num_items;
};

/**
 * @brief PD ID information advertised by the PD.
 *
 * @param version 1-Byte Manufacturer's version number
 * @param model 1-byte Manufacturer's model number
 * @param vendor_code 3-bytes IEEE assigned OUI
 * @param serial_number 4-byte serial number for the PD
 * @param firmware_version 3-byte version (major, minor, build)
 */
struct osdp_pd_id {
	int version;
	int model;
	uint32_t vendor_code;
	uint32_t serial_number;
	uint32_t firmware_version;
};

/**
 * @brief pointer to function that copies received bytes into buffer. This
 * function should be non-blocking.
 *
 * @param data for use by underlying layers. osdp_channel::data is passed
 * @param buf byte array copy incoming data
 * @param len sizeof `buf`. Can copy utmost `len` bytes into `buf`
 *
 * @retval +ve: number of bytes copied on to `buf`. Must be <= `len`
 * @retval -ve on errors
 */
typedef int (*osdp_read_fn_t)(void *data, uint8_t *buf, int maxlen);

/**
 * @brief pointer to function that sends byte array into some channel. This
 * function should be non-blocking.
 *
 * @param data for use by underlying layers. osdp_channel::data is passed
 * @param buf byte array to be sent
 * @param len number of bytes in `buf`
 *
 * @retval +ve: number of bytes sent. must be <= `len`
 * @retval -ve on errors
 */
typedef int (*osdp_write_fn_t)(void *data, uint8_t *buf, int len);

/**
 * @brief pointer to function that drops all bytes in TX/RX fifo. This
 * function should be non-blocking.
 *
 * @param data for use by underlying layers. osdp_channel::data is passed
 */
typedef void (*osdp_flush_fn_t)(void *data);

/**
 * @brief User defined communication channel abstraction for OSDP devices.
 * The methods for read/write/flush are expected to be non-blocking.
 *
 * @param data pointer to a block of memory that will be passed to the
 *             send/receive/flush method. This is optional (can be set to NULL)
 * @param id on multi-drop networks, more than one PD can share the same
 *           channel (read/write/flush pointers). On such networks, the
 *           channel_id is used to lock a PD to a channel. On multi-drop
 *           networks, this `id` must non-zero and be unique for each bus.
 * @param recv Pointer to function used to receive osdp packet data
 * @param send Pointer to function used to send osdp packet data
 * @param flush Pointer to function used to flush the channel (optional)
 */
struct osdp_channel {
	void *data;
	int id;
	osdp_read_fn_t recv;
	osdp_write_fn_t send;
	osdp_flush_fn_t flush;
};

/**
 * @brief OSDP PD Information. This struct is used to describe a PD to LibOSDP.
 *
 * @param name User provided name for this PD (log messages include this name)
 * @param baud_rate Can be one of 9600/19200/38400/57600/115200/230400
 * @param address 7 bit PD address. the rest of the bits are ignored. The
 *        special address 0x7F is used for broadcast. So there can be 2^7-1
 *        devices on a multi-drop channel
 * @param flags Used to modify the way the context is setup. See `OSDP_FLAG_XXX`
 * @param id Static information that the PD reports to the CP when it received a
 *        `CMD_ID`. These information must be populated by a PD application.
 * @param cap This is a pointer to an array of structures containing the PD'
 *        capabilities. Use { -1, 0, 0 } to terminate the array. This is used
 *        only PD mode of operation
 * @param channel Communication channel ops structure, containing send/recv
 *        function pointers.
 * @param scbk Pointer to 16 bytes of Secure Channel Base Key for the PD. If
 *        non-null, this is used to set-up the secure channel.
 */
typedef struct {
	const char *name;
	int baud_rate;
	int address;
	int flags;
	struct osdp_pd_id id;
	const struct osdp_pd_cap *cap;
	struct osdp_channel channel;
	const uint8_t *scbk;
} osdp_pd_info_t;

/**
 * @brief To keep the OSDP internal data structures from polluting the exposed
 * headers, they are typedefed to void before sending them to the upper layers.
 * This level of abstraction looked reasonable as _technically_ no one should
 * attempt to modify it outside of the LibOSDP and their definition may change
 * at any time.
 */
typedef void osdp_t;

enum osdp_status_report_type {
	/**
	 * @brief Status report of the inputs attached the PD
	 */
	OSDP_STATUS_REPORT_INPUT,
	/**
	 * @brief Status report of the output attached the PD
	 */
	OSDP_STATUS_REPORT_OUTPUT,
	/**
	 * @brief Local tamper and power status report
	 *
	 * Bit-0: tamper
	 * Bit-1: power
	 */
	OSDP_STATUS_REPORT_LOCAL,
	/**
	 * @brief Remote tamper and power status report
	 *
	 * Bit-0: tamper
	 * Bit-1: power
	 */
	OSDP_STATUS_REPORT_REMOTE,
};

/**
 * @brief Status report structure. Used by OSDP_CMD_STATUS and OSDP_EVENT_STATUS.
 * In case of command, it is used to send a query to the PD while in the case of
 * events, the PR responds back with this structure.
 *
 * This can is used by the PD to indicate various status change reports. Upto
 * a maximum of 32 statuses can be reported using this API.
 *
 * @param type The kind of event to report see `enum osdp_event_status_type_e`
 * @param nr_entries Number of valid bits in `status`
 * @param status Status bit mask
 */
struct osdp_status_report {
	enum osdp_status_report_type type;
	int nr_entries;
	uint32_t mask;
};

/* ------------------------------- */
/*         OSDP Commands           */
/* ------------------------------- */

#define OSDP_CMD_TEXT_MAX_LEN          32
#define OSDP_CMD_KEYSET_KEY_MAX_LEN    32
#define OSDP_CMD_MFG_MAX_DATALEN       64

#define OSDP_CMD_FILE_TX_FLAG_CANCEL (1UL << 31)

/**
 * @brief Command sent from CP to Control digital output of PD.
 *
 * @param output_no 0 = First Output, 1 = Second Output, etc.
 * @param control_code One of the following:
 *        0 - NOP – do not alter this output
 *        1 - set the permanent state to OFF, abort timed operation (if any)
 *        2 - set the permanent state to ON, abort timed operation (if any)
 *        3 - set the permanent state to OFF, allow timed operation to complete
 *        4 - set the permanent state to ON, allow timed operation to complete
 *        5 - set the temporary state to ON, resume perm state on timeout
 *        6 - set the temporary state to OFF, resume permanent state on timeout
 * @param timer_count Time in units of 100 ms
 */
struct osdp_cmd_output {
	uint8_t output_no;
	uint8_t control_code;
	uint16_t timer_count;
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
	OSDP_LED_COLOR_MAGENTA,
	OSDP_LED_COLOR_CYAN,
	OSDP_LED_COLOR_WHITE,
	OSDP_LED_COLOR_SENTINEL
};

/**
 * @brief LED params sub-structure. Part of LED command. See struct osdp_cmd_led
 *
 * @param control_code One of the following:
 *        Temporary Control Code:
 *           0 - NOP - do not alter this LED's temporary settings
 *           1 - Cancel any temporary operation and display this LED's permanent
 *               state immediately
 *           2 - Set the temporary state as given and start timer immediately
 *        Permanent Control Code:
 *           0 - NOP - do not alter this LED's permanent settings
 *           1 - Set the permanent state as given
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param on_color Color to set during the ON timer (enum osdp_led_color_e)
 * @param off_color Color to set during the OFF timer (enum osdp_led_color_e)
 * @param timer_count Time in units of 100 ms (only for temporary mode)
 */
struct osdp_cmd_led_params {
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t on_color;
	uint8_t off_color;
	uint16_t timer_count;
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
	struct osdp_cmd_led_params temporary;
	struct osdp_cmd_led_params permanent;
};

/**
 * @brief Sent from CP to control the behaviour of a buzzer in the PD.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param control_code 0: no tone, 1: off, 2: default tone, 3+ is TBD.
 * @param on_count The ON duration of the flash, in units of 100 ms
 * @param off_count The OFF duration of the flash, in units of 100 ms
 * @param rep_count The number of times to repeat the ON/OFF cycle; 0: forever
 */
struct osdp_cmd_buzzer {
	uint8_t reader;
	uint8_t control_code;
	uint8_t on_count;
	uint8_t off_count;
	uint8_t rep_count;
};

/**
 * @brief Command to manipulate any display units that the PD supports.
 *
 * @param reader 0 = First Reader, 1 = Second Reader, etc.
 * @param control_code One of the following:
 *        1 - permanent text, no wrap
 *        2 - permanent text, with wrap
 *        3 - temp text, no wrap
 *        4 - temp text, with wrap
 * @param temp_time duration to display temporary text, in seconds
 * @param offset_row row to display the first character (1 indexed)
 * @param offset_col column to display the first character (1 indexed)
 * @param length Number of characters in the string
 * @param data The string to display
 */
struct osdp_cmd_text {
	uint8_t reader;
	uint8_t control_code;
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
 * @param address Unit ID to which this PD will respond after the change takes
 *        effect.
 * @param baud_rate baud rate value 9600/19200/38400/57600/115200/230400
 */
struct osdp_cmd_comset {
	uint8_t address;
	uint32_t baud_rate;
};

/**
 * @brief This command transfers an encryption key from the CP to a PD.
 *
 * @param type Type of keys:
 *   - 0x01 – Secure Channel Base Key
 * @param length Number of bytes of key data - (Key Length in bits + 7) / 8
 * @param data Key data
 */
struct osdp_cmd_keyset {
	uint8_t type;
	uint8_t length;
	uint8_t data[OSDP_CMD_KEYSET_KEY_MAX_LEN];
};

/**
 * @brief Manufacturer Specific Commands
 *
 * @param vendor_code 3-byte IEEE assigned OUI. Most Significant 8-bits are
 *        unused.
 * @param command 1-byte manufacturer defined osdp command
 * @param length Length of command data (optional)
 * @param data Command data (optional)
 */
struct osdp_cmd_mfg {
	uint32_t vendor_code;
	uint8_t command;
	uint8_t length;
	uint8_t data[OSDP_CMD_MFG_MAX_DATALEN];
};

/**
 * @brief File transfer start command
 *
 * @param id Pre-agreed file ID between CP and PD.
 * @param flags Reserved and set to zero by OSDP spec;
 *
 * Note: flags upper bits are used by libosdp as:
 *   bit-31 - OSDP_CMD_FILE_TX_FLAG_CANCEL: cancel an ongoing transfer
 */
struct osdp_cmd_file_tx {
	int id;
	uint32_t flags;
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
	OSDP_CMD_MFG,
	OSDP_CMD_FILE_TX,
	OSDP_CMD_STATUS,
	OSDP_CMD_SENTINEL
};

/**
 * @brief OSDP Command Structure. This is a wrapper for all individual OSDP
 * commands.
 *
 * @param id used to select specific commands in union. Type: enum osdp_cmd_e
 * @param led LED command structure
 * @param buzzer buzzer command structure
 * @param text text command structure
 * @param output output command structure
 * @param comset comset command structure
 * @param keyset keyset command structure
 */
struct osdp_cmd {
	enum osdp_cmd_e id;
	union {
		struct osdp_cmd_led led;
		struct osdp_cmd_buzzer buzzer;
		struct osdp_cmd_text text;
		struct osdp_cmd_output output;
		struct osdp_cmd_comset comset;
		struct osdp_cmd_keyset keyset;
		struct osdp_cmd_mfg mfg;
		struct osdp_cmd_file_tx file_tx;
		struct osdp_status_report status;
	};
};

/* ------------------------------- */
/*          OSDP Events            */
/* ------------------------------- */

#define OSDP_EVENT_CARDREAD_MAX_DATALEN   64
#define OSDP_EVENT_KEYPRESS_MAX_DATALEN   64
#define OSDP_EVENT_MFGREP_MAX_DATALEN     128

/**
 * @brief Various card formats that a PD can support. This is sent to CP
 * when a PD must report a card read.
 */
enum osdp_event_cardread_format_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED,
	OSDP_CARD_FMT_RAW_WIEGAND,
	OSDP_CARD_FMT_ASCII,
	OSDP_CARD_FMT_SENTINEL
};

/**
 * @brief OSDP event cardread
 *
 * @param reader_no In context of readers attached to current PD, this number
 *        indicated this number. This is not supported by LibOSDP.
 * @param format Format of the card being read.
 *        see `enum osdp_event_cardread_format_e`
 * @param direction Card read direction of PD 0 - Forward; 1 - Backward
 * @param length Length of card data in bytes or bits depending on `format`
 *        (see note).
 * @param data Card data of `length` bytes or bits depending on `format`
 *        (see note).
 *
 * @note When `format` is set to OSDP_CARD_FMT_RAW_UNSPECIFIED or
 * OSDP_CARD_FMT_RAW_WIEGAND, the length is expressed in bits. OTOH, when it is
 * set to OSDP_CARD_FMT_ASCII, the length is in bytes. The number of bytes to
 * read from the `data` field must be interpreted accordingly.
 */
struct osdp_event_cardread {
	int reader_no;
	enum osdp_event_cardread_format_e format;
	int direction;
	int length;
	uint8_t data[OSDP_EVENT_CARDREAD_MAX_DATALEN];
};

/**
 * @brief OSDP Event Keypad
 *
 * @param reader_no In context of readers attached to current PD, this number
 *                  indicated this number. This is not supported by LibOSDP.
 * @param length Length of keypress data in bytes
 * @param data keypress data of `length` bytes
 */
struct osdp_event_keypress {
	int reader_no;
	int length;
	uint8_t data[OSDP_EVENT_KEYPRESS_MAX_DATALEN];
};

/**
 * @brief OSDP Event Manufacturer Specific Command
 *
 * Note: OSDP spec v2.2 makes this structure fixed at 4 bytes. LibOSDP allows
 * for some additional data to be passed in this command using the data and
 * length fields. To be fully compliant with the specification, set the length
 * field to 0.
 *
 * @param vendor_code 3-bytes IEEE assigned OUI of manufacturer
 * @param command 1-byte reply code
 * @param length Length of manufacturer data in bytes (optional)
 * @param data manufacturer data of `length` bytes (optional)
 */
struct osdp_event_mfgrep {
	uint32_t vendor_code;
	uint8_t command;
	uint8_t length;
	uint8_t data[OSDP_EVENT_MFGREP_MAX_DATALEN];
};

/**
 * @brief OSDP PD Events
 */
enum osdp_event_type {
	/**
	 * @brief Card read event
	 */
	OSDP_EVENT_CARDREAD = 1,
	/**
	 * @brief Key press event
	 */
	OSDP_EVENT_KEYPRESS,
	/**
	 * @brief Manufacturer specific reply event
	 */
	OSDP_EVENT_MFGREP,
	/**
	 * @brief Status event
	 */
	OSDP_EVENT_STATUS,
	/**
	 * @brief Event maximum
	 */
	OSDP_EVENT_SENTINEL
};

/**
 * @brief OSDP Event structure.
 *
 * @param id used to select specific event in union. See: enum osdp_event_type
 * @param keypress keypress event structure
 * @param cardread cardread event structure
 * @param mfgrep mfgrep event structure
 */
struct osdp_event {
	enum osdp_event_type type;
	union {
		struct osdp_event_keypress keypress;
		struct osdp_event_cardread cardread;
		struct osdp_event_mfgrep mfgrep;
		struct osdp_status_report status;
	};
};

/**
 * @brief Callback for PD command notifications. After it has been registered
 * with `osdp_pd_set_command_callback`, this method is invoked when the PD
 * receives a command from the CP.
 *
 * @param arg pointer that will was passed to the arg param of
 * `osdp_pd_set_command_callback`.
 * @param cmd pointer to the received command.
 *
 * @retval 0 if LibOSDP must send a `osdp_ACK` response
 * @retval -ve if LibOSDP must send a `osdp_NAK` response
 * @retval +ve and modify the passed `struct osdp_cmd *cmd` if LibOSDP must send
 * a specific response. This is useful for sending manufacturer specific
 * reply ``osdp_MFGREP``.
 */
typedef int (*pd_command_callback_t)(void *arg, struct osdp_cmd *cmd);

/**
 * @brief Callback for CP event notifications. After is has been registered with
 * `osdp_cp_set_event_callback`, this method is invoked when the CP receives an
 * event from the PD.
 *
 * @param arg pointer that will was passed to the arg param of
 * `osdp_cp_set_event_callback`.
 * @param pd PD offset number as in `pd_info_t *`.
 * @param ev pointer to osdp_event struct (filled by libosdp).
 *
 * @retval 0 on handling the event successfully.
 * @retval -ve on errors.
 */
typedef int (*cp_event_callback_t)(void *arg, int pd, struct osdp_event *ev);

/* ------------------------------- */
/*            PD Methods           */
/* ------------------------------- */

/**
 * @brief This method is used to setup a device in PD mode. Application must
 * store the returned context pointer and pass it back to all OSDP functions
 * intact.
 *
 * @param info Pointer to info struct populated by application.
 *
 * @retval OSDP Context on success
 * @retval NULL on errors
 */
osdp_t *osdp_pd_setup(const osdp_pd_info_t *info);

/**
 * @brief Periodic refresh method. Must be called by the application at least
 * once every 50ms to meet OSDP timing requirements.
 *
 * @param ctx OSDP context
 */
void osdp_pd_refresh(osdp_t *ctx);

/**
 * @brief Cleanup all osdp resources. The context pointer is no longer valid
 * after this call.
 *
 * @param ctx OSDP context
 */
void osdp_pd_teardown(osdp_t *ctx);

/**
 * @brief Set PD's capabilities
 *
 * @param ctx OSDP context
 * @param cap pointer to array of cap (`struct osdp_pd_cap`) terminated by a
 *        capability with cap->function_code set to 0.
 */
void osdp_pd_set_capabilities(osdp_t *ctx, const struct osdp_pd_cap *cap);

/**
 * @brief Set callback method for PD command notification. This callback is
 * invoked when the PD receives a command from the CP.
 *
 * @param ctx OSDP context
 * @param cb The callback function's pointer
 * @param arg A pointer that will be passed as the first argument of `cb`
 */
void osdp_pd_set_command_callback(osdp_t *ctx, pd_command_callback_t cb,
				  void *arg);

/**
 * @brief API to notify PD events to CP. These events are sent to the CP as an
 * alternate response to a POLL command.
 *
 * @param ctx OSDP context
 * @param event pointer to event struct. Must be filled by application.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osdp_pd_notify_event(osdp_t *ctx, const struct osdp_event *event);

/**
 * @brief Deletes all events from the PD's event queue.
 *
 * @param ctx OSDP context
 * @return int Count of events dequeued.
 */
int osdp_pd_flush_events(osdp_t *ctx);

/* ------------------------------- */
/*            CP Methods           */
/* ------------------------------- */

/**
 * @brief This method is used to setup a device in CP mode. Application must
 * store the returned context pointer and pass it back to all OSDP functions
 * intact.
 *
 * @param num_pd Number of PDs connected to this CP. The `osdp_pd_info_t *` is
 *        treated as an array of length num_pd.
 * @param info Pointer to info struct populated by application.
 *
 * @retval OSDP Context on success
 * @retval NULL on errors
 */
osdp_t *osdp_cp_setup(int num_pd, const osdp_pd_info_t *info);

/**
 * @brief Periodic refresh method. Must be called by the application at least
 * once every 50ms to meet OSDP timing requirements.
 *
 * @param ctx OSDP context
 */
void osdp_cp_refresh(osdp_t *ctx);

/**
 * @brief Cleanup all osdp resources. The context pointer is no longer valid
 * after this call.
 *
 * @param ctx OSDP context
 */
void osdp_cp_teardown(osdp_t *ctx);

/**
 * @brief Generic command enqueue API.
 *
 * @param ctx OSDP context
 * @param pd PD offset number as in `pd_info_t *`.
 * @param cmd command pointer. Must be filled by application.
 *
 * @retval 0 on success
 * @retval -1 on failure
 *
 * @note This method only adds the command on to a particular PD's command
 * queue. The command itself can fail due to various reasons.
 */
int osdp_cp_send_command(osdp_t *ctx, int pd, const struct osdp_cmd *cmd);

/**
 * @brief Get PD ID information as reported by the PD. Calling this method
 * before the CP has had a the chance to get this information will return
 * invalid/stale results.
 *
 * @param ctx OSDP context
 * @param pd PD offset number as in `pd_info_t *`.
 * @param id A pointer to struct osdp_pd_id that will be filled with the
 *           PD ID information that the PD last returned.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osdp_cp_get_pd_id(const osdp_t *ctx, int pd, struct osdp_pd_id *id);

/**
 * @brief Get capability associated to a function_code that the PD reports in
 * response to osdp_CAP(0x62) command. Calling this method before the CP has
 * had a the chance to get this information will return invalid/stale results.
 *
 * @param ctx OSDP context
 * @param pd PD offset number as in `pd_info_t *`.
 * @param cap in/out; struct osdp_pd_cap pointer with osdp_pd_cap::function_code
 *            set to the function code to get data for.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osdp_cp_get_capability(const osdp_t *ctx, int pd, struct osdp_pd_cap *cap);

/**
 * @brief Set callback method for CP event notification. This callback is
 * invoked when the CP receives an event from the PD.
 *
 * @param ctx OSDP context
 * @param cb The callback function's pointer
 * @param arg A pointer that will be passed as the first argument of `cb`
 */
void osdp_cp_set_event_callback(osdp_t *ctx, cp_event_callback_t cb, void *arg);

/**
 * @brief Set or clear OSDP public flags
 *
 * @param ctx OSDP context
 * @param pd_idx PD offset number as in `pd_info_t *`.
 * @param flags One or more of the public flags (OSDP_FLAG_XXX) exported from
 *              osdp.h. Any other bits will cause this method to fail.
 * @param do_set when true: set `flags` in ctx; when false: clear `flags` in ctx
 *
 * @retval 0 on success
 * @retval -1 on failure
 *
 * @note It doesn't make sense to call some initialization time flags during
 * runtime. This method is for dynamic flags that can be turned on/off at runtime.
 */
int osdp_cp_modify_flag(osdp_t *ctx, int pd_idx, uint32_t flags, bool do_set);

/* ------------------------------- */
/*          Common Methods         */
/* ------------------------------- */

/**
 * @brief Different levels of log messages; based on importance of the message
 * with LOG_EMERG being most critical to LOG_DEBUG being the least.
 */
enum osdp_log_level_e {
	OSDP_LOG_EMERG,
	OSDP_LOG_ALERT,
	OSDP_LOG_CRIT,
	OSDP_LOG_ERROR,
	OSDP_LOG_WARNING,
	OSDP_LOG_NOTICE,
	OSDP_LOG_INFO,
	OSDP_LOG_DEBUG,
	OSDP_LOG_MAX_LEVEL
};

/**
 * @brief Puts a string to the logging medium
 *
 * @param msg a null-terminated char buffer.
 *
 * @retval 0 on success; -ve on errors
 */
typedef int (*osdp_log_puts_fn_t)(const char *msg);

/**
 * @brief A callback function to be used with external loggers
 *
 * @param log_level A syslog style log level. See `enum osdp_log_level_e`
 * @param file Relative path to file which produced the log message
 * @param line Line number in `file` which produced the log message
 * @param msg The log message
 */
typedef void (*osdp_log_callback_fn_t)(int log_level, const char *file,
				       unsigned long line, const char *msg);

/**
 * @brief Configure OSDP Logging.
 *
 * @param name A soft name for this module; will appear in all the log lines.
 * @param log_level OSDP log levels of type `enum osdp_log_level_e`. Default is
 *                  LOG_INFO.
 * @param puts_fn A puts() like function that will be invoked to write the log
 *                buffer. Can be handy if you want to log to file on a UART
 *                device without putchar redirection. See `osdp_log_puts_fn_t`
 *                definition to see the behavioral expectations. When this is
 *                set to NULL, LibOSDP will log to stderr.
 *
 * Note: This function has to be called before osdp_{cp,pd}_setup(). Otherwise
 *       it will be ignored.
 */
void osdp_logger_init(const char *name, int log_level,
		      osdp_log_puts_fn_t puts_fn);

/**
 * @brief A callback function that gets called when LibOSDP wants to emit a log
 *        line. All messages (of all log levels) are passed on to this callback
 *        without any log formatting. This API is for users who may already have
 *        a logger configured in their application.
 *
 * @param cb The callback function. See `osdp_log_callback_fn_t` for more
 *           details.
 *
 * Note: This function has to be called before osdp_{cp,pd}_setup(). Otherwise
 *       it will be ignored.
 */
void osdp_set_log_callback(osdp_log_callback_fn_t cb);

/**
 * @brief Get LibOSDP version as a `const char *`. Used in diagnostics.
 *
 * @retval version string
 */
const char *osdp_get_version();

/**
 * @brief Get LibOSDP source identifier as a `const char *`. This string has
 * info about the source tree from which this version of LibOSDP was built. Used
 * in diagnostics.
 *
 * @retval source identifier string
 */
const char *osdp_get_source_info();

/**
 * @brief Get a bit mask of number of PD that are online currently.
 *
 * @param ctx OSDP context
 * @param bitmask pointer to an array of bytes. must be as large as
 *                (num_pds + 7 / 8).
 */
void osdp_get_status_mask(const osdp_t *ctx, uint8_t *bitmask);

/**
 * @brief Get a bit mask of number of PD that are online and have an active
 * secure channel currently.
 *
 * @param ctx OSDP context
 * @param bitmask pointer to an array of bytes. must be as large as
 *                (num_pds + 7 / 8).
 */
void osdp_get_sc_status_mask(const osdp_t *ctx, uint8_t *bitmask);

/**
 * @brief OSDP File operations struct that needs to be filled by the CP/PD
 * application and registered with LibOSDP using osdp_file_register_ops()
 * before a file transfer command can be initiated.
 */
struct osdp_file_ops {
	/**
	 * @brief A opaque pointer to private data that can be filled by the
	 * application which will be passed as the first argument for each of
	 * the below functions. Applications can keep their file context info
	 * such as the open file descriptors or any other private data here.
	 */
	void *arg;

	/**
	 * @brief Open a pre-agreed file
	 *
	 * @param arg Opaque pointer to private data passed though by libosdp
	 * @param file_id File ID of pre-agreed file between this CP and PD
	 * @param size Size of the file that was opened (filled by application)
	 *
	 * @retval 0 on success. -1 on errors.
	 */
	int (*open)(void *arg, int file_id, int *size);

	/**
	 * @brief Read a chunk of file data into buffer
	 *
	 * @param arg Opaque pointer to private data passed though by libosdp
	 * @param buf Buffer to store file data read
	 * @param size Number of bytes to read from file into buffer
	 * @param offset Number of bytes from the beginning of the file to
	 *               start reading from.
	 *
	 * @retval Number of bytes read, 0 on EOF, -ve on errors.
	 *
	 * @note LibOSDP will guarantee that size and offset params are always
	 * positive and size is always greater than or equal to offset.
	 */
	int (*read)(void *arg, void *buf, int size, int offset);

	/**
	 * @brief Write a chunk of file data from buffer to disk.
	 *
	 * @param arg Opaque pointer to private data passed though by libosdp
	 * @param buf Buffer with file data to be stored to disk
	 * @param size Number of bytes to write to disk
	 * @param offset Number of bytes from the beginning of the file to
	 *               start writing too.
	 *
	 * @retval Number of bytes read, 0 on EOF, -ve on errors.
	 *
	 * @note LibOSDP will guarantee that size and offset params are always
	 * positive and size is always greater than or equal to offset.
	 */
	int (*write)(void *arg, const void *buf, int size, int offset);

	/**
	 * @brief Close file that corresponds to a given file descriptor
	 *
	 * @param arg Opaque pointer to private data passed though by libosdp
	 *
	 * @retval 0 on success. -1 on errors.
	 */
	int (*close)(void *arg);
};

/**
 * @brief Register a global file operations struct with OSDP. Both CP and PD
 * modes should have done so already before CP can sending a OSDP_CMD_FILE_TX.
 *
 * @param ctx OSDP context
 * @param pd PD number in case of CP. This param is ignored in PD mode
 * @param ops Populated file operations struct
 *
 * @retval 0 on success. -1 on errors.
 */
int osdp_file_register_ops(osdp_t *ctx, int pd_idx,
			   const struct osdp_file_ops *ops);

/**
 * @brief Query file transfer status if one is in progress. Calling this method
 * when there is no file transfer progressing will return error.
 *
 * @param ctx OSDP context
 * @param pd PD number in case of CP. This param is ignored in PD mode
 * @param size Total size of the file (as obtained from file_ops->open())
 * @param offset Offset into the file that has been sent/received (CP/PD)
 * @retval 0 on success. -1 on errors.
 */
int osdp_get_file_tx_status(const osdp_t *ctx, int pd_idx,
			    int *size, int *offset);

#ifdef __cplusplus
}
#endif

#endif	/* _OSDP_H_ */
