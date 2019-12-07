/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDPCTL_COMMON_H_
#define _OSDPCTL_COMMON_H_

#include <stdint.h>
#include <osdp.h>

#define CONFIG_FILE_PATH_LENGTH		128

enum config_osdp_mode_e {
	CONFIG_OSDP_MODE_CP,
	CONFIG_OSDP_MODE_PD
};

enum config_channel_topology_e {
	CONFIG_CHANNEL_TOPOLOGY_CHAIN,
	CONFIG_CHANNEL_TOPOLOGY_STAR
};

enum config_channel_type_e {
	CONFIG_CHANNEL_TYPE_UART,
	CONFIG_CHANNEL_TYPE_UNIX,
	CONFIG_CHANNEL_TYPE_INTERNAL
};

struct config_s {
	/* GLOBAL */
	int osdp_mode;
	char *channel_device;
	int channel_type;
	int channel_topology;
	int channel_speed;

	/* CP */
	int num_pd;
	uint8_t master_key[16];

	/* PD */
	int pd_address;
	int vendor_code;
	int model;
	int version;
	int serial_number;

	/* cli_args */
	int dump_config;
	char *config_file;
};

extern struct config_s g_config;

void config_print(struct config_s *config);
void config_parse(const char *filename, struct config_s *config);

// from strutils.c

int atohstr(char *hstr, const uint8_t *arr, const int arr_len);
int hstrtoa(uint8_t *arr, const char *hstr);

// command handlers
int cmd_handler_start(int argc, char *argv[], struct config_s *c);

#endif
