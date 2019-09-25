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

#define byte_0(x)                    (uint8_t)((x)&0xFF)
#define byte_1(x)                    (uint8_t)(((x)>> 8)&0xFF)
#define byte_2(x)                    (uint8_t)(((x)>>16)&0xFF)
#define byte_3(x)                    (uint8_t)(((x)>>24)&0xFF)

/* casting helpers */
#define to_osdp(p)                   ((osdp_t *)p)
#define to_cp(p)                     (((osdp_t *)(p))->cp)
#define to_pd(p, i)                  (((osdp_t *)(p))->pd + i)
#define to_ctx(p)                    ((osdp_t *)p->__parent)
#define child_set_parent(c, p)       ((c)->__parent = (void *)(p))
#define to_current_pd(p)             (to_cp(p)->current_pd)

#define sizeof_array(x)              (sizeof(x)/sizeof(x[0]))

#define set_current_pd(p, i)                                \
    do {                                                    \
        to_cp(p)->current_pd = to_pd(p, i);                 \
        to_cp(p)->pd_offset = i;                            \
    } while (0)

/* OSDP reserved commands */
#define CMD_POLL                     0x60
#define CMD_ID                       0x61
#define CMD_CAP                      0x62
#define CMD_DIAG                     0x63
#define CMD_LSTAT                    0x64
#define CMD_ISTAT                    0x65
#define CMD_OSTAT                    0x66
#define CMD_RSTAT                    0x67
#define CMD_OUT                      0x68
#define CMD_LED                      0x69
#define CMD_BUZ                      0x6A
#define CMD_TEXT                     0x6B
#define CMD_RMODE                    0x6C
#define CMD_TDSET                    0x6D
#define CMD_COMSET                   0x6E
#define CMD_DATA                     0x6F
#define CMD_XMIT                     0x70
#define CMD_PROMPT                   0x71
#define CMD_SPE                      0x72
#define CMD_BIOREAD                  0x73
#define CMD_BIOMATCH                 0x74
#define CMD_KEYSET                   0x75
#define CMD_CHLNG                    0x76
#define CMD_SCRYPT                   0x77
#define CMD_CONT                     0x79
#define CMD_ABORT                    0x7A
#define CMD_MAXREPLY                 0x7B
#define CMD_MFG                      0x80
#define CMD_SCDONE                   0xA0
#define CMD_XWR                      0xA1

/* OSDP reserved responses */
#define REPLY_ACK                    0x40
#define REPLY_NAK                    0x41
#define REPLY_PDID                   0x45
#define REPLY_PDCAP                  0x46
#define REPLY_LSTATR                 0x48
#define REPLY_ISTATR                 0x49
#define REPLY_OSTATR                 0x4A
#define REPLY_RSTATR                 0x4B
#define REPLY_RAW                    0x50
#define REPLY_FMT                    0x51
#define REPLY_PRES                   0x52
#define REPLY_KEYPPAD                0x53
#define REPLY_COM                    0x54
#define REPLY_SCREP                  0x55
#define REPLY_SPER                   0x56
#define REPLY_BIOREADR               0x57
#define REPLY_BIOMATCHR              0x58
#define REPLY_CCRYPT                 0x76
#define REPLY_RMAC_I                 0x78
#define REPLY_MFGREP                 0x90
#define REPLY_BUSY                   0x79
#define REPLY_XRD                    0xB1

/* Global flags */
#define FLAG_CP_MODE                 0x00000001 /* Set when initialized as CP */

/* CP flags */
#define CP_FLAG_INIT_DONE            0x00000001 /* set after data is malloc-ed and initialized */

/* PD Flags */
#define PD_FLAG_SC_CAPABLE           0x00000001 /* PD secure channel capable */
#define PD_FLAG_TAMPER               0x00000002 /* local tamper status */
#define PD_FLAG_POWER                0x00000004 /* local power status */
#define PD_FLAG_R_TAMPER             0x00000008 /* remote tamper status */
#define PD_FLAG_COMSET_INPROG        0x00000010 /* set when comset is enabled */
#define PD_FLAG_AWAIT_RESP           0x00000020 /* set after command is sent */
#define PD_FLAG_SKIP_SEQ_CHECK       0x00000040 /* disable seq checks (debug) */
#define PD_FLAG_PD_MODE              0x80000000 /* device is setup as PD */
typedef uint64_t millis_t;

/* CMD_OUT */
enum cmd_output_control_code {
    CMD_OP_NOP,
    CMD_OP_POFF,
    CMD_OP_PON,
    CMD_OP_POFF_T,
    CMD_OP_PON_T,
    CMD_OP_TON,
    CMD_OP_TOFF,
};

struct cmd_output {
    uint8_t output_no;
    uint8_t control_code;
    uint16_t tmr_count;
};

/* CMD_LED */
enum hmi_cmd_temp_ctrl_code_e {
    TEMP_CC_TEMP_NOP,
    TEMP_CC_TEMP_CANCEL,
    TEMP_CC_TEMP_SET
};

enum hmi_cmd_perm_ctrl_code_e {
    TEMP_CC_PERM_NOP,
    TEMP_CC_PERM_SET
};

struct _led_params {
    uint8_t control_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t on_color;
    uint8_t off_color;
    uint16_t timer;
};

struct cmd_led {
    uint8_t reader;
    uint8_t number;
    struct _led_params temperory;
    struct _led_params permanent;
};

/* CMD_BUZ */
enum buzzer_tone_code_e {
    TONE_NONE,
    TONE_OFF,
    TONE_DEFAULT,
};

struct cmd_buzzer {
    uint8_t reader;
    uint8_t tone_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t rep_count;
};

/* CMD_TEXT */
enum text_command_e {
    PERM_TEXT_NO_WRAP=1,
    PERM_TEXT_WRAP,
    TEMP_TEXT_NO_WRAP,
    TEMP_TEXT_WRAP
};

struct cmd_text {
    uint8_t reader;
    uint8_t cmd;
    uint8_t temp_time;
    uint8_t offset_row;
    uint8_t offset_col;
    uint8_t length;
    uint8_t data[32];
};

/* CMD_COMSET */
struct cmd_comset {
    uint8_t addr;
    uint32_t baud;
};

struct cmd {
    uint8_t len;
    uint8_t id;
    uint8_t data[0];
};

union cmd_all {
    struct cmd_led led;
    struct cmd_buzzer buzzer;
    struct cmd_text text;
    struct cmd_output output;
    struct cmd_comset comset;
};

typedef struct {
    /* OSDP specified data */
    void *__parent;
    int baud_rate;
    int address;
    int seq_number;
    struct pd_cap cap[CAP_SENTINEL];
    struct pd_id id;

    /* PD state management */
    int state;
    int flags;
    millis_t tstamp;
    int phy_state;
    struct cmd_queue *queue;
    uint8_t scratch[OSDP_PD_SCRATCH_SIZE];
    millis_t phy_tstamp;

    /* callbacks */
    int (*send_func)(uint8_t *buf, int len);
    int (*recv_func)(uint8_t *buf, int len);
} pd_t;

struct cmd_queue {
    int head;
    int tail;
    uint8_t buffer[OSDP_PD_CMD_QUEUE_SIZE];
};

typedef struct {
    void *__parent;
    int num_pd;
    int state;
    int flags;

    pd_t *current_pd;  /* current operational pd's pointer */
    int pd_offset;     /* current pd's offset into ctx->pd */

    /* callbacks */
    int (*keypress_handler)(int address, uint8_t key);
    int (*cardread_handler)(int address, int format, uint8_t *data, int len);
} cp_t;

typedef struct {
    int magic;
    int flags;

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

enum pd_nak_code_e {
    PD_NAK_NONE,
    PD_NAK_MSG_CHK,
    PD_NAK_CMD_LEN,
    PD_NAK_CMD_UNKNOWN,
    PD_NAK_SEQ_NUM,
    PD_NAK_SC_UNSUP,
    PD_NAK_SC_COND,
    PD_NAK_BIO_TYPE,
    PD_NAK_BIO_FMT,
    PD_NAK_RECORD,
    PD_NAK_SENTINEL
};

/* from phy.c */
int phy_build_packet_head(pd_t *p, uint8_t *buf, int maxlen);
int phy_build_packet_tail(pd_t *p, uint8_t *buf, int len, int maxlen);
int phy_decode_packet(pd_t *p, uint8_t *buf, int blen);
const char *get_nac_reason(int code);
void phy_state_reset(pd_t *pd);

/* from common.c */
millis_t millis_now();
millis_t millis_since(millis_t last);
uint8_t compute_checksum(uint8_t *msg, int length);
uint16_t compute_crc16(uint8_t *data, int  len);
void osdp_dump(const char *head, const uint8_t *data, int len);
void osdp_log(int log_level, const char *fmt, ...);
void osdp_set_log_level(int log_level);

#endif
