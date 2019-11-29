/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <osdp.h>

#include "uart.h"
#include "arg-parser.h"

struct osdpctl_opts_s {
	int pd_address;
	int baud_rate;
	char device[64];
	char mode[16];
	uint8_t master_key[16];
	int help;
};

struct osdpctl_opts_s osdpctl_opts;

void osdp_dump(const char *head, const uint8_t * data, int len);

int validate_opt_baudrate(void *data)
{
	struct osdpctl_opts_s *p = data;

	if (p->baud_rate != 9600 &&
	    p->baud_rate != 38400 &&
	    p->baud_rate != 115200) {
		printf("Invalid baudrate %d\n", p->baud_rate);
		return -1;
	}
	return 0;
}

int validate_uart_device(void *data)
{
	struct osdpctl_opts_s *p = data;

	if (access(p->device, F_OK) == -1) {
		printf("UART device does not exist %s\n", p->device);
		return -1;
	}
	return 0;
}

int validate_osdp_mode(void *data)
{
	struct osdpctl_opts_s *p = data;

	if (strcmp(p->mode, "CP") != 0 &&
	    strcmp(p->mode, "PD") != 0) {
		printf("Invalid osdp mode %s\n", p->mode);
		return -1;
	}
	return 0;
}

struct ap_option ap_opts[] = {
	{
		AP_ARG('b', "baud-rate"),
		AP_STORE_INT(struct osdpctl_opts_s, baud_rate),
		AP_FLAGS(AP_OPT_REQUIRED),
		AP_VALIDATOR(validate_opt_baudrate),
		AP_HELP("set baud rate for <DEVICE>")
	},
	{
		AP_ARG('a', "pd-addr"),
		AP_STORE_INT(struct osdpctl_opts_s, pd_address),
		AP_FLAGS(AP_OPT_NOFLAG),
		AP_VALIDATOR(validate_opt_baudrate),
		AP_HELP("set PD address")
	},
	{
		AP_ARG('d', "device"),
		AP_STORE_STR(struct osdpctl_opts_s, device, 64),
		AP_FLAGS(AP_OPT_REQUIRED),
		AP_VALIDATOR(validate_uart_device),
		AP_HELP("UART device")
	},
	{
		AP_ARG('m', "mode"),
		AP_STORE_STR(struct osdpctl_opts_s, mode, 16),
		AP_FLAGS(AP_OPT_REQUIRED),
		AP_VALIDATOR(validate_osdp_mode),
		AP_HELP("OSDP Mode, can be 'CP' or 'PD'")
	},
	{
		AP_ARG('k', "key"),
		AP_STORE_HEX(struct osdpctl_opts_s, master_key, 16),
		AP_FLAGS(AP_OPT_REQUIRED),
		AP_VALIDATOR(NULL),
		AP_HELP("Master key hex string (32 chars)")
	},
	{
		AP_OPT_SENTINEL
	}
};

int main(int argc, char *argv[])
{
	int ret;

	ap_init("osdpctl", "Control and Manage OSDP devices");

	ret = ap_parse(argc, argv, ap_opts, &osdpctl_opts);
	if (ret != 0) {
		printf("Error parsing cmd args\n");
		exit(-1);
	}

	if(uart_init(osdpctl_opts.device, osdpctl_opts.baud_rate) < 0)
		return -1;

	osdp_pd_info_t info[] = {
		{
			.address = osdpctl_opts.pd_address,
			.baud_rate = osdpctl_opts.baud_rate,
			.init_flags = 0,
			.send_func = uart_write,
			.recv_func = uart_read
		},
	};

	osdp_cp_t *ctx = osdp_cp_setup(1, info, osdpctl_opts.master_key);
	if (ctx == NULL) {
		printf("cp init failed!\n");
		return -1;
	}

	osdp_set_log_level(7);

	while (1)
	{
		// your application code.

		osdp_cp_refresh(ctx);
		usleep(1000);
	}
	return 0;
}
