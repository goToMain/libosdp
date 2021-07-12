/*
 * Copyright (c) 2022 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_TRS_H_
#define _OSDP_TRS_H_

#include "osdp_common.h"

#ifdef CONFIG_OSDP_TRS

int osdp_trs_cmd_build(struct osdp_pd *pd, uint8_t *buf, int max_len);
int osdp_trs_reply_decode(struct osdp_pd *pd, uint8_t *buf, int len);
int osdp_trs_reply_build(struct osdp_pd *pd, uint8_t *buf, int max_len);
int osdp_trs_cmd_decode(struct osdp_pd *pd, uint8_t *buf, int len);

#else /* CONFIG_OSDP_TRS */

static inline int osdp_trs_cmd_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);
	return -1;
}
static inline int osdp_trs_reply_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -1;
}
static inline int osdp_trs_reply_build(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);
	return -1;
}
static inline int osdp_trs_cmd_decode(struct osdp_pd *pd, uint8_t *buf, int len)
{
	ARG_UNUSED(pd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -1;
}

#endif /* CONFIG_OSDP_TRS */

#endif /* _OSDP_TRS_H_ */