/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "common.h"

int cmd_handler_send(int argc, char *argv[], void *data)
{
	struct config_s *c = data;

	if (argc < 1) {
		printf ("Error: must pass a config file\n");
		return -1;
	}
	config_parse(argv[0], c);


	return 0;
}
