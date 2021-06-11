/*
 * Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

extern int (*test_cp_build_packet)(struct osdp_pd *);
extern int (*test_pd_decode_packet)(struct osdp_pd *pd, int *len);
int test_check_command(void *arg, struct osdp_cmd *cmd);

struct {
	osdp_t *cp_ctx;
	osdp_t *pd_ctx;
} priv;

int test_osdp_commands_setup(struct test *t)
{
	/* mock CP */
	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 0,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = NULL,
		.channel.recv = NULL,
		.channel.flush = NULL,
		.scbk = NULL,
	};
	osdp_logger_init(t->loglevel, printf);
	priv.cp_ctx = (struct osdp *) osdp_cp_setup(1, &info, NULL);
	if (priv.cp_ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	SET_CURRENT_PD(priv.cp_ctx, 0);

	/* mock PD */
	struct osdp_pd_cap cap[] = {
		{
			.function_code = OSDP_PD_CAP_COMMUNICATION_SECURITY,
			.compliance_level = 1,
			.num_items = 1
		}, {
			.function_code = OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,
			.compliance_level = 1,
			.num_items = 2
		}, {
			.function_code = OSDP_PD_CAP_OUTPUT_CONTROL,
			.compliance_level = 1,
			.num_items = 2
		}, {
			.function_code = OSDP_PD_CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 2
		}, {
			.function_code = OSDP_PD_CAP_READER_TEXT_OUTPUT,
			.compliance_level = 1,
			.num_items = 1
		},
		{ -1, 0, 0 }
	};
	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 0,
		.flags = 0,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = cap,
		.channel.data = NULL,
		.channel.send = NULL,
		.channel.recv = NULL,
		.scbk = NULL,
	};

	priv.pd_ctx = (struct osdp *) osdp_pd_setup(&info_pd);
	if (priv.pd_ctx == NULL) {
		printf(SUB_1 "pd init failed!\n");
		osdp_cp_teardown((osdp_t *) priv.cp_ctx);
		return -1;
	}

	osdp_pd_set_command_callback(
		priv.pd_ctx,
		test_check_command,
		&priv
	);

	t->mock_data = (void *)&priv;

	return 0;
}

void test_osdp_commands_teardown(struct test *t)
{
	ARG_UNUSED(t);

	osdp_cp_teardown(priv.cp_ctx);
	osdp_pd_teardown(priv.pd_ctx);
}

struct test_osdp_commands {
	int command;
	int reply;
} g_test_osdp_commands[] = {
	{ CMD_POLL,	REPLY_ACK },
	{ CMD_ID,	REPLY_PDID },
	{ CMD_CAP,	REPLY_PDCAP },
	{ CMD_LSTAT,	REPLY_LSTATR },
	{ CMD_RSTAT,	REPLY_RSTATR },
	{ CMD_OUT,	REPLY_ACK },
	{ CMD_LED,	REPLY_ACK },
	{ CMD_BUZ,	REPLY_ACK },
	{ CMD_TEXT,	REPLY_ACK },
	{ CMD_CHLNG,	REPLY_CCRYPT },
	{ CMD_SCRYPT,	REPLY_RMAC_I },
	{ CMD_MFG,	REPLY_ACK },
	{ CMD_COMSET,	REPLY_COM },
	{ CMD_ACURXSIZE, REPLY_ACK },
	{ CMD_KEEPACTIVE, REPLY_ACK },
	{ CMD_ABORT,	REPLY_ACK },
};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define __ASSERT(cond, str) if (!(cond)) { printf(SUB_2 "Assert %s failed!\n", str); return -1; }
#define ASSERT(cond) __ASSERT((cond), TOSTRING(cond))

int test_check_command(void *arg, struct osdp_cmd *cmd)
{
	ARG_UNUSED(arg);

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT:
		ASSERT(cmd->output.output_no == 1);
		ASSERT(cmd->output.control_code == 1);
		ASSERT(cmd->output.timer_count == 10)
		break;
	case OSDP_CMD_LED:
		ASSERT(cmd->led.reader == 0);
		ASSERT(cmd->led.led_number == 1);
		ASSERT(cmd->led.temporary.control_code == 2);
		ASSERT(cmd->led.temporary.on_count == 100);
		ASSERT(cmd->led.temporary.off_count == 100);
		ASSERT(cmd->led.temporary.timer_count == 10000);
		ASSERT(cmd->led.permanent.control_code == 1);
		ASSERT(cmd->led.permanent.on_count == 100);
		ASSERT(cmd->led.permanent.off_count == 100);
		break;
	case OSDP_CMD_BUZZER:
		ASSERT(cmd->buzzer.reader == 0);
		ASSERT(cmd->buzzer.control_code == 2);
		ASSERT(cmd->buzzer.on_count == 100);
		ASSERT(cmd->buzzer.off_count == 100);
		ASSERT(cmd->buzzer.rep_count == 10)
		break;
	case OSDP_CMD_TEXT:
		ASSERT(cmd->text.reader == 0);
		ASSERT(cmd->text.control_code == 1);
		ASSERT(cmd->text.temp_time == 0);
		ASSERT(cmd->text.offset_row == 1);
		ASSERT(cmd->text.offset_col == 1);
		ASSERT(cmd->text.length == 7);
		if (strncmp((char *)cmd->text.data, "LibOSDP", 7) != 0)
			return -1;
		break;
	case OSDP_CMD_COMSET:
		ASSERT(cmd->comset.address == 73);
		ASSERT(cmd->comset.baud_rate == 115200);
		break;
	case OSDP_CMD_MFG:
		ASSERT(cmd->mfg.vendor_code == 13);
		ASSERT(cmd->mfg.command == 153);
		ASSERT(cmd->mfg.length == 7);
		if (strncmp((char *)cmd->mfg.data, "LibOSDP", 7) != 0)
			return -1;
		break;
	default:
		return -1;
	}

	return 0;
}

void test_fill_comand(int cmd, void *data)
{
	struct osdp_cmd c;

	memset(&c, 0, sizeof(struct osdp_cmd));
	c.id = cmd;
	switch (cmd) {
	case CMD_OUT:
		c.output.output_no = 1;
		c.output.control_code = 1;
		c.output.timer_count = 10;
		break;
	case CMD_LED:
		c.led.reader = 0;
		c.led.led_number = 1;
		c.led.temporary.control_code = 2;
		c.led.temporary.on_count = 100;
		c.led.temporary.off_count = 100;
		c.led.temporary.timer_count = 10000;
		c.led.permanent.control_code = 1;
		c.led.permanent.on_count = 100;
		c.led.permanent.off_count = 100;
		break;
	case CMD_BUZ:
		c.buzzer.reader = 0;
		c.buzzer.control_code = 2;
		c.buzzer.on_count = 100;
		c.buzzer.off_count = 100;
		c.buzzer.rep_count = 10;
		break;
	case CMD_TEXT:
		c.text.reader = 0;
		c.text.control_code = 1;
		c.text.temp_time = 0;
		c.text.offset_row = 1;
		c.text.offset_col = 1;
		c.text.length = 7;
		strncpy((char *)c.text.data, "LibOSDP", 7);
		break;
	case CMD_COMSET:
		c.comset.address = 73;
		c.comset.baud_rate = 115200;
		break;
	case CMD_MFG:
		c.mfg.vendor_code = 13;
		c.mfg.command = 153;
		c.mfg.length = 7;
		strncpy((char *)c.mfg.data, "LibOSDP", 7);
		break;
	default:
		break;
	}
	memcpy(data, &c, sizeof(struct osdp_cmd));
}

void run_osdp_commands_tests(struct test *t)
{
	int i, asize, ret, tmp;
	struct test_osdp_commands *c;

	printf("\nStarting OSDP Commands test\n");

	printf(SUB_1 "setting up OSDP devices\n");
	if (test_osdp_commands_setup(t))
		return;

	struct osdp_pd *cp_pd = TO_PD(priv.cp_ctx, 0);
	struct osdp_pd *pd_pd = TO_PD(priv.pd_ctx, 0);

	printf(SUB_1 "Testing commands\n");

	asize = ARRAY_SIZEOF(g_test_osdp_commands);
	for (i = 0; i < asize; i++) {
		c = &g_test_osdp_commands[i];

		cp_pd->cmd_id = c->command;
		test_fill_comand(c->command, cp_pd->ephemeral_data);
		if (test_cp_build_packet(cp_pd)) {
			TEST_REPORT(t, false);
			printf(SUB_1 "Faild to build OSDP command %x\n", c->command);
			continue;
		}

		memcpy(pd_pd->rx_buf, cp_pd->rx_buf, cp_pd->rx_buf_len);
		pd_pd->rx_buf_len = cp_pd->rx_buf_len;
		pd_pd->reply_id = 0; /* reset past reply ID so phy can send NAK */
		pd_pd->ephemeral_data[0] = 0; /* reset past NAK reason */
		ret = test_pd_decode_packet(pd_pd, &tmp);
		if (ret) {
			TEST_REPORT(t, false);
			printf(SUB_2 "Faild to decode OSDP command %x ret: %d\n",
			       c->command, ret);
			continue;
		}

		if (pd_pd->reply_id != c->reply) {
			TEST_REPORT(t, false);
			printf(SUB_2 "Invalid Reply(%x) for Cmd(%d). Expected Reply(%x)",
			       pd_pd->reply_id, c->command, c->reply);
			continue;
		}

		TEST_REPORT(t, true);
	}

	printf(SUB_1 "OSDP Commands test complete\n");

	test_osdp_commands_teardown(t);
}
