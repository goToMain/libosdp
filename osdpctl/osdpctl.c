/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "arg_parser.h"
#include "uart.h"
#include "common.h"

#define MOVE_ARGS(end) argc-end-1, argv+end+1

struct config_s g_config;

struct ap_option ap_opts[] = {
	{
		AP_ARG('c', "config-file", "file"),
		AP_STORE_STR(struct config_s, config_file),
		AP_FLAGS(AP_OPT_REQUIRED),
		AP_VALIDATOR(NULL),
		AP_HELP("Config file (ini format)")
	},
	AP_CMD("start", "Start a osdp service"),
	AP_SENTINEL
};

int main(int argc, char *argv[])
{
	int opt_end;

        ap_init("osdpctl", "Setup/Manage OSDP devices");

	opt_end = ap_parse(argc, argv, ap_opts, &g_config);

	config_parse(g_config.config_file, &g_config);

	if (argv[opt_end] == NULL) {
		printf("Error: no command specified\n");
		ap_print_help(ap_opts, -1);
	}

	if (strcmp("start", argv[opt_end]) == 0) {
		cmd_handler_start(MOVE_ARGS(opt_end), &g_config);
	}

	return 0;
}
