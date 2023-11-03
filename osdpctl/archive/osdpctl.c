/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "arg_parser.h"
#include "common.h"

struct config_s g_config;

static void print_version()
{
	printf("%s [%s]\n", osdp_get_version(), osdp_get_source_info());
}

struct ap_option ap_opts[] = {
	{ AP_ARG('l', "log-file", "file"),
	  AP_STORE_STR(struct config_s, log_file), AP_FLAGS(AP_OPT_NOFLAG),
	  AP_VALIDATOR(NULL), AP_HELP("Log to file instead of tty") },
	{ AP_CMD("start", cmd_handler_start), AP_HELP("Start a osdp service") },
	{ AP_CMD("send", cmd_handler_send),
	  AP_HELP("Send a command to a osdp device") },
	{ AP_CMD("stop", cmd_handler_stop),
	  AP_HELP("Stop a service started earlier") },
	{ AP_CMD("check", cmd_handler_check),
	  AP_HELP("Check and print parsed config") },
	{
		AP_ARG_BOOL('v', "version", print_version, "Print Version"),
	},
	AP_SENTINEL
};

static void signal_handler(int sig)
{
	if (sig == SIGINT || sig == SIGHUP || sig == SIGTERM)
		exit(EXIT_FAILURE);
}

void cleanup()
{
	int i;
	struct config_pd_s *pd;

	if (g_config.service_started) {
		unlink(g_config.pid_file);
		stop_cmd_server(&g_config);
	}

	for (i = 0; i < g_config.num_pd; i++) {
		pd = g_config.pd + i;
		channel_close(&g_config.chn_mgr, pd->channel_device);
	}

	channel_manager_teardown(&g_config.chn_mgr);
}

void osdpctl_process_init()
{
	struct sigaction sigact;

	atexit(cleanup);
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
	sigaction(SIGHUP, &sigact, (struct sigaction *)NULL);
	sigaction(SIGTERM, &sigact, (struct sigaction *)NULL);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	osdpctl_process_init();

	ap_init("osdpctl", "Setup/Manage OSDP devices");

	if (argc < 2) {
		printf("Error: must provide a config file!\n");
		exit(-1);
	}

	channel_manager_init(&g_config.chn_mgr);

	config_parse(argv[1], &g_config);

	return ap_parse(argc - 1, argv + 1, ap_opts, &g_config);
}
