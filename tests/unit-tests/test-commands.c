/*
 * Copyright (c) 2021-2025 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <osdp.h>
#include "test.h"

/* Test context for command tests */
struct test_command_ctx {
	osdp_t *cp_ctx;
	osdp_t *pd_ctx;
	int cp_runner;
	int pd_runner;

	/* Command tracking */
	bool cmd_seen;
	int last_cmd_id;
	void *last_cmd_data;

	/* Event tracking */
	bool event_seen;
	int last_event_type;
	void *last_event_data;

	/* Manufacturer command specific */
	bool mfg_reply_expected;
	uint32_t mfg_vendor_code;
	uint8_t mfg_data[64];
	int mfg_data_len;
};

static struct test_command_ctx g_test_ctx = {0};

int test_commands_event_callback(void *arg, int pd, struct osdp_event *ev)
{
	ARG_UNUSED(pd);
	struct test_command_ctx *ctx = arg;

	ctx->event_seen = true;
	ctx->last_event_type = ev->type;

	if (ev->type == OSDP_EVENT_MFGREP) {
		ctx->last_event_data = malloc(sizeof(struct osdp_event));
		memcpy(ctx->last_event_data, ev, sizeof(struct osdp_event));
	}

	return 0;
}

int test_commands_command_callback(void *arg, struct osdp_cmd *cmd)
{
	struct test_command_ctx *ctx = arg;

	ctx->cmd_seen = true;
	ctx->last_cmd_id = cmd->id;

	/* Handle manufacturer commands specially */
	if (cmd->id == OSDP_CMD_MFG) {
		if (ctx->mfg_reply_expected &&
		    cmd->mfg.vendor_code == ctx->mfg_vendor_code &&
		    cmd->mfg.length == ctx->mfg_data_len &&
		    memcmp(cmd->mfg.data, ctx->mfg_data, ctx->mfg_data_len) == 0) {
			/* Return positive value to trigger MFGREP */
			return 1;
		}
	}

	/* Handle status commands - provide mock response */
	if (cmd->id == OSDP_CMD_STATUS) {
		/* We don't actually do anything with the status command in this test,
		 * just acknowledge it was received */
		return 0;
	}

	/* Handle comset commands - acknowledge and wait for comset done */
	if (cmd->id == OSDP_CMD_COMSET) {
		/* COMSET requires special handling - we need to acknowledge it */
		return 0;
	}

	return 0;
}

static int setup_test_environment(struct test *t)
{
	printf(SUB_1 "setting up OSDP devices\n");

	if (test_setup_devices(t, &g_test_ctx.cp_ctx, &g_test_ctx.pd_ctx)) {
		printf(SUB_1 "Failed to setup devices!\n");
		return -1;
	}

	osdp_cp_set_event_callback(g_test_ctx.cp_ctx, test_commands_event_callback, &g_test_ctx);
	osdp_pd_set_command_callback(g_test_ctx.pd_ctx, test_commands_command_callback, &g_test_ctx);

	printf(SUB_1 "starting async runners\n");

	g_test_ctx.cp_runner = async_runner_start(g_test_ctx.cp_ctx, osdp_cp_refresh);
	g_test_ctx.pd_runner = async_runner_start(g_test_ctx.pd_ctx, osdp_pd_refresh);

	if (g_test_ctx.cp_runner < 0 || g_test_ctx.pd_runner < 0) {
		printf(SUB_1 "Failed to created CP/PD runners\n");
		return -1;
	}

	/* Wait for devices to come online */
	int rc = 0;
	uint8_t status = 0;
	while (1) {
		if (rc > 10) {
			printf(SUB_1 "PD failed to come online\n");
			return -1;
		}
		osdp_get_status_mask(g_test_ctx.cp_ctx, &status);
		if (status & 1)
			break;
		usleep(1000 * 1000);
		rc++;
	}

	return 0;
}

static void teardown_test_environment()
{
	printf(SUB_1 "tearing down test environment\n");

	async_runner_stop(g_test_ctx.cp_runner);
	async_runner_stop(g_test_ctx.pd_runner);

	osdp_cp_teardown(g_test_ctx.cp_ctx);
	osdp_pd_teardown(g_test_ctx.pd_ctx);

	/* Clean up any allocated event data */
	if (g_test_ctx.last_event_data) {
		free(g_test_ctx.last_event_data);
		g_test_ctx.last_event_data = NULL;
	}

	memset(&g_test_ctx, 0, sizeof(g_test_ctx));
}

static void reset_test_state()
{
	g_test_ctx.cmd_seen = false;
	g_test_ctx.last_cmd_id = 0;
	g_test_ctx.event_seen = false;
	g_test_ctx.last_event_type = 0;
	g_test_ctx.mfg_reply_expected = false;

	if (g_test_ctx.last_event_data) {
		free(g_test_ctx.last_event_data);
		g_test_ctx.last_event_data = NULL;
	}
}

static bool wait_for_command(int expected_cmd_id, int timeout_sec)
{
	int rc = 0;
	while (rc < timeout_sec) {
		if (g_test_ctx.cmd_seen && g_test_ctx.last_cmd_id == expected_cmd_id) {
			return true;
		}
		usleep(1000 * 1000);
		rc++;
	}
	return false;
}

static bool wait_for_event(int expected_event_type, int timeout_sec)
{
	int rc = 0;
	while (rc < timeout_sec) {
		if (g_test_ctx.event_seen && g_test_ctx.last_event_type == expected_event_type) {
			return true;
		}
		usleep(1000 * 1000);
		rc++;
	}
	return false;
}

static bool test_buzzer_command()
{
	printf(SUB_2 "testing buzzer command\n");
	reset_test_state();

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

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send buzzer command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_BUZZER, 5);
}

static bool test_led_command()
{
	printf(SUB_2 "testing LED command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_LED,
		.led = {
			.reader = 0,
			.led_number = 0,
			.temporary = {
				.control_code = 1,
				.on_count = 10,
				.off_count = 10,
				.on_color = OSDP_LED_COLOR_RED,
				.off_color = OSDP_LED_COLOR_NONE,
				.timer_count = 100,
			},
		},
	};

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send LED command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_LED, 5);
}

static bool test_output_command()
{
	printf(SUB_2 "testing output command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_OUTPUT,
		.output = {
			.output_no = 0,
			.control_code = 1,
			.timer_count = 100,
		},
	};

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send output command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_OUTPUT, 5);
}

static bool test_text_command()
{
	printf(SUB_2 "testing text command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_TEXT,
		.text = {
			.reader = 0,
			.control_code = 1,
			.temp_time = 30,
			.offset_row = 1,
			.offset_col = 1,
			.length = 7,
		},
	};
	strcpy((char *)cmd.text.data, "LibOSDP");

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send text command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_TEXT, 5);
}

static bool test_mfg_command_simple()
{
	printf(SUB_2 "testing manufacturer command (simple)\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_MFG,
		.mfg = {
			.vendor_code = 0x00030201,
			.length = 10,
		},
	};
	uint8_t test_data[] = {9,1,9,2,6,3,1,7,7,0};
	memcpy(cmd.mfg.data, test_data, sizeof(test_data));

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send mfg command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_MFG, 5);
}

static bool test_mfg_command_with_reply()
{
	printf(SUB_2 "testing manufacturer command with reply\n");
	reset_test_state();

	/* Set up expected manufacturer reply parameters */
	g_test_ctx.mfg_reply_expected = true;
	g_test_ctx.mfg_vendor_code = 0x00030201;
	g_test_ctx.mfg_data_len = 10;
	uint8_t test_data[] = {9,1,9,2,6,3,1,7,7,0};
	memcpy(g_test_ctx.mfg_data, test_data, sizeof(test_data));

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_MFG,
		.mfg = {
			.vendor_code = g_test_ctx.mfg_vendor_code,
			.length = g_test_ctx.mfg_data_len,
		},
	};
	memcpy(cmd.mfg.data, g_test_ctx.mfg_data, g_test_ctx.mfg_data_len);

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send mfg command with reply\n");
		return false;
	}

	/* Wait for command to be received */
	if (!wait_for_command(OSDP_CMD_MFG, 5)) {
		printf(SUB_2 "MFG command not received by PD\n");
		return false;
	}

	/* Wait for manufacturer reply event */
	if (!wait_for_event(OSDP_EVENT_MFGREP, 5)) {
		printf(SUB_2 "MFGREP event not received by CP\n");
		return false;
	}

	/* Verify the manufacturer reply event data */
	if (g_test_ctx.last_event_data) {
		struct osdp_event *ev = (struct osdp_event *)g_test_ctx.last_event_data;
		if (ev->mfgrep.vendor_code != g_test_ctx.mfg_vendor_code ||
		    ev->mfgrep.length != g_test_ctx.mfg_data_len ||
		    memcmp(ev->mfgrep.data, g_test_ctx.mfg_data, g_test_ctx.mfg_data_len) != 0) {
			printf(SUB_2 "MFGREP event data mismatch\n");
			return false;
		}
	} else {
		printf(SUB_2 "MFGREP event data not captured\n");
		return false;
	}

	return true;
}

static bool test_led_permanent_command()
{
	printf(SUB_2 "testing LED command (permanent mode)\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_LED,
		.led = {
			.reader = 1,
			.led_number = 0,
			.permanent = {
				.control_code = 1,
				.on_count = 10,
				.off_count = 10,
				.on_color = OSDP_LED_COLOR_RED,
				.off_color = OSDP_LED_COLOR_NONE,
			},
		},
	};

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send LED permanent command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_LED, 5);
}

static bool test_comset_command()
{
	printf(SUB_2 "testing communication set command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_COMSET,
		.comset = {
			.address = 101,
			.baud_rate = 9600,
		},
	};

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send comset command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_COMSET, 5);
}

static bool test_keyset_command()
{
	printf(SUB_2 "testing key set command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_KEYSET,
		.keyset = {
			.type = 1,
			.length = 16,
		},
	};
	uint8_t key_data[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	memcpy(cmd.keyset.data, key_data, sizeof(key_data));

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send keyset command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_KEYSET, 5);
}

static bool test_status_command()
{
	printf(SUB_2 "testing status command\n");
	reset_test_state();

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_STATUS,
		.status = {
			.type = OSDP_STATUS_REPORT_INPUT,
		},
	};

	if (osdp_cp_submit_command(g_test_ctx.cp_ctx, 0, &cmd)) {
		printf(SUB_2 "Failed to send status command\n");
		return false;
	}

	return wait_for_command(OSDP_CMD_STATUS, 5);
}

void run_command_tests(struct test *t)
{
	bool overall_result = true;

	printf("\nBegin Command Tests (pytest-style)\n");

	/* Setup test environment once */
	if (setup_test_environment(t) != 0) {
		printf(SUB_1 "Failed to setup test environment\n");
		TEST_REPORT(t, false);
		return;
	}

	printf(SUB_1 "running command tests\n");

	/* Run all command tests */
	overall_result &= test_buzzer_command();
	overall_result &= test_led_command();
	overall_result &= test_led_permanent_command();
	overall_result &= test_output_command();
	overall_result &= test_text_command();
	overall_result &= test_keyset_command();
	overall_result &= test_mfg_command_simple();
	overall_result &= test_mfg_command_with_reply();

	/* Teardown test environment */
	teardown_test_environment();

	printf(SUB_1 "Command tests %s\n", overall_result ? "succeeded" : "failed");
	TEST_REPORT(t, overall_result);
}
