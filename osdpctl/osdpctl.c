/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "arg_parser.h"
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
	{
		AP_ARG('d', "dump-config", ""),
		AP_STORE_BOOL(struct config_s, dump_config),
		AP_FLAGS(AP_OPT_NOFLAG),
		AP_VALIDATOR(NULL),
		AP_HELP("Print parsed config and exit")
	},
	AP_CMD("start", "Start a osdp service"),
	AP_SENTINEL
};

static void signal_handler(int sig)
{
	if (sig == SIGINT)
		exit(EXIT_FAILURE);
}

void cleanup()
{
	int i;
	struct config_pd_s *pd;

	for (i = 0; i < g_config.num_pd; i++) {
		pd = g_config.pd + i;
		channel_teardown(pd);
	}
}

void osdpctl_process_init()
{
	struct sigaction sigact;

	atexit(cleanup);
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}

int main(int argc, char *argv[])
{
	int opt_end;

	osdpctl_process_init();

        ap_init("osdpctl", "Setup/Manage OSDP devices");

	opt_end = ap_parse(argc, argv, ap_opts, &g_config);

	config_parse(g_config.config_file, &g_config);

	if (g_config.dump_config) {
		config_print(&g_config);
		exit(0);
	}

	if (argv[opt_end] == NULL) {
		printf("Error: no command specified\n");
		ap_print_help(ap_opts, -1);
	}

	if (strcmp("start", argv[opt_end]) == 0) {
		cmd_handler_start(MOVE_ARGS(opt_end), &g_config);
	}

	return 0;
}
