/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>
#include <stddef.h>

#define OSDP_PD_ERR_RETRY_SEC             (60)
#define OSDP_PD_POLL_TIMEOUT_MS           (50)
#define OSDP_PD_CMD_QUEUE_SIZE            (128)
#define OSDP_RESP_TOUT_MS                 (400)
#define OSDP_PD_SCRATCH_SIZE              (64)
#define OSDP_PD_CAP_SENTINEL              { -1, 0, 0 } /* struct pd_cap[] end */

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

enum osdp_cmd_output_control_code {
    CMD_OP_NOP,
    CMD_OP_POFF,
    CMD_OP_PON,
    CMD_OP_POFF_T,
    CMD_OP_PON_T,
    CMD_OP_TON,
    CMD_OP_TOFF,
};

enum osdp_cmd_temp_ctrl_code_e {
    TEMP_CC_TEMP_NOP,
    TEMP_CC_TEMP_CANCEL,
    TEMP_CC_TEMP_SET
};

enum osdp_cmd_perm_ctrl_code_e {
    TEMP_CC_PERM_NOP,
    TEMP_CC_PERM_SET
};

enum osdp_cmd_buzzer_tone_code_e {
    TONE_NONE,
    TONE_OFF,
    TONE_DEFAULT,
};

enum osdp_cmd_text_command_e {
    PERM_TEXT_NO_WRAP=1,
    PERM_TEXT_WRAP,
    TEMP_TEXT_NO_WRAP,
    TEMP_TEXT_WRAP
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
    uint8_t function_code; /* enum osdp_pd_cap_function_code_e */
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
    int (*send_func)(uint8_t *buf, int len);

    /**
     * recv_func - Copies received bytes into buffer
     * @buf - byte array copy incoming data
     * @len - sizeof `buf`. Can copy utmost `len` number of bytes into `buf`
     *
     * Returns:
     *  +ve: number of bytes copied on to `bug`. Must be <= `len`
     */
    int (*recv_func)(uint8_t *buf, int len);

} osdp_pd_info_t;

/**
 * The keep the OSDP internal data strucutres from polluting the exposed headers,
 * they are typecasted to void * before sending them to the upper layers. This
 * level of abstaction looked reasonable as _technically_ no one should attempt
 * to modify it outside fo the LibOSDP.
 */
typedef void * osdp_cp_t;
typedef void * osdp_pd_t;

/* --- CP ---- */
osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info);
void osdp_cp_refresh(osdp_cp_t *ctx);
void osdp_cp_teardown(osdp_cp_t *ctx);

int osdp_send_cmd_output(osdp_cp_t *ctx, int pd, struct osdp_cmd_output *p);
int osdp_send_cmd_led(osdp_cp_t *ctx, int pd, struct osdp_cmd_led *p);
int osdp_send_cmd_buzzer(osdp_cp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
int osdp_send_cmd_comset(osdp_cp_t *ctx, int pd, struct osdp_cmd_comset *p);

/* --- PD --- */
osdp_pd_t *osdp_pd_setup(int num_pd, osdp_pd_info_t *p);
void osdp_pd_teardown(osdp_pd_t *ctx);

int osdp_pd_set_led_handler(osdp_pd_t *ctx, int (*led)(struct osdp_cmd_led *p));
int osdp_pd_set_buzzer_handler(osdp_pd_t *ctx, int (*buzzer)(struct osdp_cmd_buzzer *p));
int osdp_pd_set_output_handler(osdp_pd_t *ctx, int (*output)(struct osdp_cmd_output *p));
int osdp_pd_set_text_handler(osdp_pd_t *ctx, int (*text)(struct osdp_cmd_text *p));
int osdp_pd_set_comset_handler(osdp_pd_t *ctx, int (*comset)(struct osdp_cmd_comset *p));

#endif /* _OSDP_H_ */
