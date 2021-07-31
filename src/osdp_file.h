/*
 * Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_FILE_H_
#define _OSDP_FILE_H_

#include "osdp_common.h"

#define TO_FILE(pd) (pd)->file

/**
 * @brief OSDP specified command: File Transer:
 *
 * @param type File transfer type
 *        - 1: opaque file contents recognizable by this specific PD
 *        - 2..127: Reserved for future use
 *        - 128..255: Reserved for private use
 * @param size File size (4 bytes,) little-endian format.
 * @param offset Offset in file of current message.
 * @param length Length of data section in this command.
 * @param data File contents. Variable length
 */
struct osdp_cmd_file_xfer {
	uint8_t type;
	uint32_t size;
	uint32_t offset;
	uint16_t length;
	uint8_t data[];
} __packed;

/**
 * @brief OSDP specified command: File Transfer Stat:
 *
 * @param control Control flags.
 *        - bit-0: 1 = OK to interleave; 0 = dedicate for filetransfer
 *        - bit-1: 1 = shall leave secure channel for file transfer; 0 = stay in
 *                 secure channel if SC is active
 *        - bit-2: 1 = eparate poll response is available; 0=no other activity
 * @param delay Request CP for a time delay in milliseconds before next
 *        CMD_FILETRANSFER message
 * @param status File transfer status. This is a signed little- endian number
 *        -  0: ok to proceed
 *        -  1: file contents processed
 *        -  2: rebooting now, expect full communications reset
 *        -  3: PD is finishing file transfer. PD should send CMD_FILETRANSFER
 *              with data length set to 0 (idle) until this status changes
 *        - -1: abort file transfer
 *        - -2: unrecognized file contents
 *        - -3: file data unacceptable (malformed)
 * @param rx_size Alternate maximum message size for CMD_FILETRANSFER. If set to
 *        0 then no change requested, otherwise use this value
 */
struct osdp_cmd_file_stat {
	uint8_t control;
	uint16_t delay;
	int16_t status;
	uint16_t rx_size;
} __packed;

enum file_tx_state_e {
	OSDP_FILE_IDLE,
	OSDP_FILE_INPROG,
	OSDP_FILE_DONE,
};

struct osdp_file {
	uint32_t flags;
	int file_id;
	enum file_tx_state_e state;
	int length;
	int size;
	int offset;
	int errors;
	struct osdp_file_ops ops;
};

#ifdef CONFIG_OSDP_FILE

int osdp_file_cmd_tx_build(struct osdp_pd *pd, uint8_t *buf, int max_len);
int osdp_file_cmd_tx_decode(struct osdp_pd *pd, uint8_t *buf, int len);
int osdp_file_cmd_stat_decode(struct osdp_pd *pd, uint8_t *buf, int len);
int osdp_file_cmd_stat_build(struct osdp_pd *pd, uint8_t *buf, int max_len);
int osdp_file_tx_initiate(struct osdp_pd *pd, int file_id, uint32_t flags);
bool osdp_file_tx_pending(struct osdp_pd *pd);
void osdp_file_tx_abort(struct osdp_pd *pd);

#else /* CONFIG_OSDP_FILE */

static inline int osdp_file_cmd_tx_build(struct osdp_pd *pd, uint8_t *buf,
					 int max_len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);
	return -1;
}
static inline int osdp_file_cmd_tx_decode(struct osdp_pd *pd, uint8_t *buf,
					  int len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -1;
}
static inline int osdp_file_cmd_stat_decode(struct osdp_pd *pd, uint8_t *buf,
					    int len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -1;
}
static inline int osdp_file_cmd_stat_build(struct osdp_pd *pd, uint8_t *buf,
					   int max_len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);
	return 0;
}
static inline int osdp_file_tx_initiate(struct osdp_pd *pd, int file_id,
					uint32_t flags)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(file_id);
	ARG_UNUSED(flags);
	return -1;
}
static inline bool osdp_file_tx_pending(struct osdp_pd *pd)
{
	ARG_UNUSED(pd);
	return false;
}

static inline void osdp_file_tx_abort(struct osdp_pd *pd)
{
	ARG_UNUSED(pd);
}

#endif /* CONFIG_OSDP_FILE */

#endif /* _OSDP_FILE_H_ */