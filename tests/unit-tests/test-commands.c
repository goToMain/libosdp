/*
 * Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <osdp.h>
#include "test.h"

struct test_data {
	bool cmd_seen;
};

int event_callback(void *arg, int pd, struct osdp_event *ev)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(pd);
	ARG_UNUSED(ev);
	return 0;
}

int command_callback(void *arg, struct osdp_cmd *cmd)
{
	ARG_UNUSED(cmd);
	struct test_data *d = arg;
	d->cmd_seen = true;
	return 0;
}

struct osdp_cmd test_make_cmd(int cmd_id)
{
	if (cmd_id == OSDP_CMD_KEYSET) {
		struct osdp_cmd cmd = {
			.id = OSDP_CMD_KEYSET,
			.keyset = {
				.type = 1,
				.length = 16,
				.data = { 0 },
			},
		};
		return cmd;
	} else {
		struct osdp_cmd cmd = {
			.id = OSDP_CMD_BUZZER,
			.buzzer = {
				.control_code = 1,
				.on_count = 10,
				.off_count = 10,
				.reader = 0,
				.rep_count = 1,
			},
		};
		return cmd;
	}
}

void run_command_tests(struct test *t)
{
	int rc;
	osdp_t *cp_ctx, *pd_ctx;
	int cp_runner = -1, pd_runner = -1;
	uint8_t status = 0;
	struct test_data d = {0};
	bool result = false;

	printf("\nBegin Commands\n");

	printf(SUB_1 "setting up OSDP devices\n");

	if (test_setup_devices(t, &cp_ctx, &pd_ctx)) {
		printf(SUB_1 "Failed to setup devices!\n");
		goto error;
	}

	osdp_cp_set_event_callback(cp_ctx, event_callback, &d);
	osdp_pd_set_command_callback(pd_ctx, command_callback, &d);

	printf(SUB_1 "starting async runners\n");

	cp_runner = async_runner_start(cp_ctx, osdp_cp_refresh);
	pd_runner = async_runner_start(pd_ctx, osdp_pd_refresh);

	if (cp_runner < 0 || pd_runner < 0) {
		printf(SUB_1 "Failed to created CP/PD runners\n");
		goto error;
	}

	rc = 0;
	while (1) {
		if (rc > 10) {
			printf(SUB_1 "PD failed to come online");
			goto error;
		}
		osdp_get_status_mask(cp_ctx, &status);
		if (status & 1)
			break;
		usleep(1000 * 1000);
		rc++;
	}

	printf(SUB_1 "sending Buzzer command\n");

	struct osdp_cmd cmd = test_make_cmd(OSDP_CMD_BUZZER);

	if (osdp_cp_submit_command(cp_ctx, 0, &cmd)) {
		printf(SUB_1 "Failed to send command\n");
		goto error;
	}

	rc = 0;
	while (1) {
		if (rc > 10) {
			printf(SUB_1 "PD failed to check command");
			goto error;
		}
		if (d.cmd_seen)
			break;
		usleep(1000 * 1000);
		rc++;
	}

	result = d.cmd_seen;

	printf(SUB_1 "Commands %s\n",
	       result ? "succeeded" : "failed");
error:
	async_runner_stop(cp_runner);
	async_runner_stop(pd_runner);

	osdp_cp_teardown(cp_ctx);
	osdp_pd_teardown(pd_ctx);

	TEST_REPORT(t, result);
}
