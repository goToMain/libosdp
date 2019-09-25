/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>

/* --- masked objects ---- */
typedef void * osdp_cp_t;
typedef void * osdp_pd_t;

#define OSDP_PD_ERR_RETRY_SEC             (60)
#define OSDP_PD_POLL_TIMEOUT_MS           (50)
#define OSDP_PD_CMD_QUEUE_SIZE            (128)
#define OSDP_RESP_TOUT_MS                 (400)
#define OSDP_PD_SCRATCH_SIZE              (64)

enum osdp_card_formats_e {
    OSDP_CARD_FMT_RAW_UNSPECIFIED,
    OSDP_CARD_FMT_RAW_WIEGAND,
    OSDP_CARD_FMT_ASCII,
    OSDP_CARD_FMT_SENTINEL
};

enum pd_cap_function_code_e {
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

struct pd_cap {
    uint8_t function_code;
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
    int baud_rate;
    int address;
    int init_flags;

    struct pd_id id;
    struct pd_cap *cap;

    int (*send_func)(uint8_t *buf, int len);
    int (*recv_func)(uint8_t *buf, int len);
} osdp_pd_info_t;

/* --- CP ---- */
osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info);
void osdp_cp_refresh(osdp_cp_t *ctx);
void osdp_cp_teardown(osdp_cp_t *ctx);

int osdp_set_output(osdp_cp_t *ctx, int pd, int output_no, int control_code, int timer);
int osdp_set_led(osdp_cp_t *ctx, int pd, int on_color, int off_color,
                 int on_count, int off_count, int rep_count, int is_permantent);
int osdp_set_buzzer(osdp_cp_t *ctx, int pd, int on_count, int off_count, int rep_count);
int osdp_set_text(osdp_cp_t *ctx, int pd, int cmd_code, int duration, int row,
                  int col, const char *msg);
int osdp_set_params(osdp_cp_t *ctx, int pd, int pd_address, uint32_t baud_rate);

/* --- PD --- */
osdp_pd_t *osdp_pd_setup(int num_pd, osdp_pd_info_t *p);
void osdp_pd_teardown(osdp_pd_t *ctx);

#endif /* _OSDP_H_ */
