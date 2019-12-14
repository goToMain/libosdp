/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "common.h"

int cmd_handler_stop(int argc, char *argv[], void *data)
{
	int pid;
	struct config_s *c = data;

	if (argc < 1) {
		printf ("Error: must pass a config file\n");
		return -1;
	}
	config_parse(argv[0], c);

	if (read_pid(c->pid_file, &pid)) {
		printf("Failed to read PID. Service not running\n");
		return -1;
	}

	kill((pid_t)pid, SIGHUP);
	return 0;
}

int cmd_handler_check(int argc, char *argv[], void *data)
{
	struct config_s *c = data;

	if (argc < 1) {
		printf ("Error: must pass a config file\n");
		return -1;
	}
	config_parse(argv[0], c);
	config_print(c);
	return 0;
}
