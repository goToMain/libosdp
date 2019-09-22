/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#ifndef _OSDP_H_
#define _OSDP_H_

#include <stdint.h>

typedef void * osdp_cp_t;

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

typedef struct {
    int baud_rate;
    int address;
    int init_flags;
    int (*send_func)(uint8_t *buf, int len);
    int (*recv_func)(uint8_t *buf, int len);
} osdp_pd_info_t;

/* --- CP ---- */
osdp_cp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info);
void osdp_cp_refresh(osdp_cp_t *ctx);
void osdp_cp_teardown(osdp_cp_t *ctx);

#endif /* _OSDP_H_ */
