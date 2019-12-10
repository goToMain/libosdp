/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "common.h"

int channel_custom_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return 0;
}

int channel_custom_recv(void *data, uint8_t *buf, int max_len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);

	return 0;
}

void channel_custom_flush(void *data)
{
	ARG_UNUSED(data);
}

int channel_custom_setup(void **data, struct config_pd_s *c)
{
	ARG_UNUSED(data);
	ARG_UNUSED(c);

	return 0;
}

void channel_custom_teardown(void *data)
{
	ARG_UNUSED(data);
}

struct channel_ops_s channel_custom = {
	.send = channel_custom_send,
	.recv = channel_custom_recv,
	.flush = channel_custom_flush,
	.setup = channel_custom_setup,
	.teardown = channel_custom_teardown
};
