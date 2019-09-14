/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#ifndef _OSDP_COMMON_H_
#define _OSDP_COMMON_H_

#include <osdp.h>

#ifndef NULL
#define NULL                         ((void *)0)
#endif

#define TRUE                         (1)
#define FALSE                        (0)
#define isset_flag(p, f)             (((p)->flags & (f)) == (f))
#define set_flag(p, f)               ((p)->flags |= (f))
#define clear_flag(p, f)             ((p)->flags &= ~(f))

#define BYTE_0(x)                    (uint8_t)((x)&0xFF)
#define BYTE_1(x)                    (uint8_t)(((x)>> 8)&0xFF)
#define BYTE_2(x)                    (uint8_t)(((x)>>16)&0xFF)
#define BYTE_3(x)                    (uint8_t)(((x)>>24)&0xFF)

/* casting helpers */
#define to_osdp(p)                   ((osdp_t *)p)
#define to_cp(p)                     ((to_osdp(p))->cp)
#define to_pd(p, i)                  ((to_osdp(p))->pd + i)
#define to_current_pd(p)             (to_cp(p) ? to_cp(p)->current_pd : (p)->pd)

#define set_current_pd(d, p)         (to_current_pd(d) = (p))

typedef struct {
    int pd_id;
    int state;
    int flags;
    int baud_rate;
    int address;
    int seq_number;

    int (*send_func)(uint8_t *buf, int len);
    int (*recv_func)(uint8_t *buf, int len);
} pd_t;

typedef struct {
    int num_pd;
    int state;
    int flags;

    pd_t *current_pd;
} cp_t;

typedef struct {
    int magic;
    int flags;
    int log_level;

    cp_t *cp;
    pd_t *pd;
} osdp_t;

enum log_levels_e {
    LOG_EMERG,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
};

uint8_t compute_checksum(uint8_t *msg, int length);
uint16_t compute_crc16(uint8_t *data, int  len);
void print(osdp_t *ctx, int log_level, const char *fmt, ...);

#endif
