/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <utils/procutils.h>

#include "common.h"

int cmd_handler_stop(int argc, char *argv[], void *data)
{
	int pid;
	struct config_s *c = data;

	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	if (read_pid(c->pid_file, &pid)) {
		printf("Failed to read PID. Service not running\n");
		return -1;
	}

	if (kill((pid_t)pid, SIGHUP) == -1) {
		perror("Failed to stop service.");
	}

	unlink(c->pid_file);
	return 0;
}

int cmd_handler_check(int argc, char *argv[], void *data)
{
	struct config_s *c = data;

	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	config_print(c);
	return 0;
}
