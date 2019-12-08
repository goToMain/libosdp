/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

int cmd_handler_start(int argc, char *argv[], struct config_s *c)
{
	int i;
	osdp_pd_info_t *info_arr, *info;
	struct config_pd_s *pd;
	int (*send_func)(uint8_t * buf, int len) = NULL;
	int (*recv_func)(uint8_t * buf, int len) = NULL;
	osdp_cp_t *cp_ctx;
	osdp_pd_t *pd_ctx;

        ARG_UNUSED(argc);
        ARG_UNUSED(argv);

	info_arr = calloc(c->num_pd, sizeof(osdp_pd_info_t));
	if (info_arr == NULL)
	{
		printf("Failed to alloc info struct\n");
		return -1;
	}

	for (i = 0; i < c->num_pd; i++)
	{
		info = info_arr + i;
		pd = c->pd + i;
		info->address = pd->address;
		info->baud_rate = pd->channel_speed;
		info->send_func = send_func;
		info->recv_func = recv_func;

		if (c->mode == CONFIG_MODE_CP)
			continue;

		memcpy(&info->id, &pd->id, sizeof(struct pd_id));
		info->cap = pd->cap;
	}

	if (c->mode == CONFIG_MODE_CP)
		cp_ctx = osdp_cp_setup(c->num_pd, info, c->cp.master_key);
	else
		pd_ctx = osdp_pd_setup(info, NULL);

	while (1)
	{
		if (c->mode == CONFIG_MODE_CP)
			osdp_cp_refresh(cp_ctx);
		else
			osdp_pd_refresh(pd_ctx);
		usleep(1000);
	}

	return 0;
}
