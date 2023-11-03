/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <utils/memory.h>
#include <utils/strutils.h>

#include "ini_parser.h"
#include "common.h"

#define INI_SUCCESS 1
#define INI_FAILURE 0

int config_parse_key_mode(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp(val, "CP") == 0) {
		p->mode = CONFIG_MODE_CP;
	} else if (strcmp(val, "PD") == 0) {
		p->pd = calloc(1, sizeof(struct config_pd_s));
		if (p->pd == NULL) {
			printf("Error: PD alloc failed!\n");
			exit(-1);
		}
		p->num_pd = 1;
		p->pd->is_pd_mode = 1;
		p->mode = CONFIG_MODE_PD;
	} else {
		return INI_FAILURE;
	}

	return INI_SUCCESS;
}

int config_parse_key_log_level(const char *val, void *data)
{
	struct config_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->log_level = i;

	return INI_SUCCESS;
}

int config_parse_key_channel_topology(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp(val, "chain") == 0)
		p->conn_topology = CONFIG_CHANNEL_TOPOLOGY_CHAIN;
	else if (strcmp(val, "star") == 0)
		p->conn_topology = CONFIG_CHANNEL_TOPOLOGY_STAR;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

int config_parse_key_pid_file(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp("", val) == 0)
		return INI_FAILURE;

	p->pid_file = strdup(val);

	return INI_SUCCESS;
}

int config_parse_key_log_file(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp("", val) == 0)
		return INI_FAILURE;

	p->log_file = strdup(val);

	return INI_SUCCESS;
}

int config_parse_key_name(const char *val, void *data)
{
	struct config_pd_s *p = data;

	if (!val || val[0] == 0)
		p->name = NULL;
	else
		p->name = strdup(val);
	return INI_SUCCESS;
}

int config_parse_key_capabilites(const char *val, void *data)
{
	int i, ival[3];
	char *tok1, *rest1, *tok2, *rest2, str[128];
	struct config_pd_s *p = data;

	safe_strncpy(str, val, sizeof(str));
	remove_all(str, ' ');
	tok1 = strtok_r(str, "[)]", &rest1);
	while (tok1 != NULL) {
		i = 0;
		ival[0] = 0;
		tok2 = strtok_r(tok1, "(,", &rest2);
		while (tok2 != NULL) {
			if (i > 2 || safe_atoi(tok2, &ival[i++]))
				return INI_FAILURE;
			tok2 = strtok_r(NULL, ", ", &rest2);
		}
		if (ival[0] <= 0 || ival[0] >= OSDP_PD_CAP_SENTINEL ||
		    ival[1] < 0 || ival[2] < 0)
			return INI_FAILURE;
		p->cap[ival[0]].function_code = ival[0];
		p->cap[ival[0]].compliance_level = ival[1];
		p->cap[ival[0]].num_items = ival[2];
		tok1 = strtok_r(NULL, "[)]", &rest1);
	}

	return INI_SUCCESS;
}

int config_parse_key_channel_type(const char *val, void *data)
{
	struct config_pd_s *p = data;

	p->channel_type = channel_guess_type(val);
	if (p->channel_type == CHANNEL_TYPE_ERR)
		return INI_FAILURE;

	return INI_SUCCESS;
}

int config_parse_key_channel_speed(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int baud;

	if (safe_atoi(val, &baud))
		return INI_FAILURE;

	if (baud != 9600 && baud != 19200 && baud != 38400 && baud != 57600 &&
	    baud != 115200 && baud != 230400) {
		printf("Error: invalid baudrate %d\n", baud);
		return INI_FAILURE;
	}
	p->channel_speed = baud;

	return INI_SUCCESS;
}

int config_parse_key_channel_device(const char *val, void *data)
{
	struct config_pd_s *p = data;

	if (!val || strlen(val) == 0)
		return INI_FAILURE;

	p->channel_device = safe_strdup(val);
	return INI_SUCCESS;
}

int config_parse_key_num_pd(const char *val, void *data)
{
	struct config_s *p = data;
	int num_pd;

	if (safe_atoi(val, &num_pd))
		return INI_FAILURE;

	if (num_pd == 0)
		return INI_FAILURE;

	if (p->mode == CONFIG_MODE_PD) {
		if (num_pd != 1) {
			printf("Error: num_pd must be 1 for PD mode\n");
			return INI_FAILURE;
		}
	} else {
		p->pd = calloc(num_pd, sizeof(struct config_pd_s));
		if (p->pd == NULL) {
			printf("Error: PD alloc failed!\n");
			exit(-1);
		}
	}

	p->num_pd = num_pd;

	return INI_SUCCESS;
}

int config_parse_key_address(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int addr;

	if (safe_atoi(val, &addr))
		return INI_FAILURE;

	if (addr < 0 || addr > 127)
		return INI_FAILURE;
	p->address = addr;

	return INI_SUCCESS;
}

int config_parse_key_key_store(const char *val, void *data)
{
	struct config_pd_s *p = data;

	if (strcmp("", val) == 0)
		return INI_FAILURE;
	p->key_store = strdup(val);

	return INI_SUCCESS;
}

int config_parse_key_vendor_code(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->id.vendor_code = (uint32_t)i;

	return INI_SUCCESS;
}

int config_parse_key_model(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->id.model = i;

	return INI_SUCCESS;
}

int config_parse_key_version(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->id.version = i;

	return INI_SUCCESS;
}

int config_parse_serial_number(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->id.serial_number = (uint32_t)i;

	return INI_SUCCESS;
}

int confif_parse_firmware_version(const char *val, void *data)
{
	struct config_pd_s *p = data;
	int i;

	if (safe_atoi(val, &i))
		return INI_FAILURE;
	p->id.firmware_version = (uint32_t)i;

	return INI_SUCCESS;
}

int config_parse_key_scbk(const char *val, void *data)
{
	uint8_t tmp[128];
	struct config_pd_s *p = data;

	if (hstrtoa(tmp, val) < 0) {
		printf("Error: failed to parse key\n");
		return INI_FAILURE;
	}
	memcpy(p->scbk, tmp, 16);

	return INI_SUCCESS;
}

struct config_key_s {
	const char *key;
	int (*handler)(const char *val, void *data);
};

const struct config_key_s g_config_key_global[] = {
	{ "mode", config_parse_key_mode },
	{ "num_pd", config_parse_key_num_pd },
	{ "log_level", config_parse_key_log_level },
	{ "conn_topology", config_parse_key_channel_topology },
	{ "pid_file", config_parse_key_pid_file },
	{ "log_file", config_parse_key_log_file },
	{ NULL, NULL }
};

const struct config_key_s g_config_key_pd[] = {
	{ "name", config_parse_key_name },
	{ "capabilities", config_parse_key_capabilites },
	{ "channel_type", config_parse_key_channel_type },
	{ "channel_speed", config_parse_key_channel_speed },
	{ "channel_device", config_parse_key_channel_device },
	{ "address", config_parse_key_address },
	{ "key_store", config_parse_key_key_store },
	{ "vendor_code", config_parse_key_vendor_code },
	{ "model", config_parse_key_model },
	{ "version", config_parse_key_version },
	{ "serial_number", config_parse_serial_number },
	{ "firmware_version", confif_parse_firmware_version },
	{ "scbk", config_parse_key_scbk },
	{ NULL, NULL }
};

const char *cap_names[OSDP_PD_CAP_SENTINEL] = {
	[OSDP_PD_CAP_UNUSED] = "NULL",
	[OSDP_PD_CAP_CONTACT_STATUS_MONITORING] = "contact_status_monitoring",
	[OSDP_PD_CAP_OUTPUT_CONTROL] = "output_control",
	[OSDP_PD_CAP_CARD_DATA_FORMAT] = "card_data_format",
	[OSDP_PD_CAP_READER_LED_CONTROL] = "reader_led_control",
	[OSDP_PD_CAP_READER_AUDIBLE_OUTPUT] = "reader_audible_control",
	[OSDP_PD_CAP_READER_TEXT_OUTPUT] = "reader_text_output",
	[OSDP_PD_CAP_TIME_KEEPING] = "time_keeping",
	[OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT] = "check_character_support",
	[OSDP_PD_CAP_COMMUNICATION_SECURITY] = "communication_security",
	[OSDP_PD_CAP_RECEIVE_BUFFERSIZE] = "receive_buffersize",
	[OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE] =
		"largest_combined_message_size",
	[OSDP_PD_CAP_SMART_CARD_SUPPORT] = "smart_card_support",
	[OSDP_PD_CAP_READERS] = "readers",
	[OSDP_PD_CAP_BIOMETRICS] = "biometrics"
};

int config_key_parse(const char *key, const char *val,
		     const struct config_key_s *p, void *data)
{
	while (p && p->key) {
		if (strcmp(p->key, key) == 0 && p->handler) {
			return p->handler(val, data);
		}
		p++;
	}

	return INI_FAILURE;
}

int config_ini_cb(void *data, const char *sec, const char *key, const char *val)
{
	int id;
	struct config_s *p = data;
	struct config_pd_s *pd;

	if (strcmp("GLOBAL", sec) == 0)
		return config_key_parse(key, val, g_config_key_global, data);
	if (strncmp("PD", sec, 3) == 0) {
		if (p->pd == NULL)
			return INI_FAILURE;
		return config_key_parse(key, val, g_config_key_pd,
					(void *)p->pd);
	}
	if (strncmp("PD-", sec, 3) == 0) {
		if (p->pd == NULL)
			return INI_FAILURE;
		if (safe_atoi(sec + 3, &id))
			return INI_FAILURE;
		pd = p->pd + id;

		return config_key_parse(key, val, g_config_key_pd, (void *)pd);
	}

	return INI_FAILURE;
}

void config_parse(const char *filename, struct config_s *config)
{
	char *rp;
	int ret = ini_parse(filename, config_ini_cb, config);

	if (ret == -1) {
		printf("Error: unable to open file: %s\n", filename);
		exit(-1);
	}
	if (ret == -2) {
		printf("Error: memory alloc failed when parsing: %s\n",
		       filename);
		exit(-1);
	}
	if (ret < 0) {
		printf("Error: in file: %s\n", filename);
		exit(-1);
	}
	if (ret > 0) {
		printf("Error: parsing file %s at line %d\n", filename, ret);
		exit(-1);
	}

	if (filename[0] != '/') {
		rp = realpath(filename, NULL);
		if (rp == NULL) {
			printf("Error: no absolute path for %s", filename);
			exit(-1);
		}
		config->config_file = rp;
	} else {
		config->config_file = safe_strdup(filename);
	}

	if (config->pd->channel_type == CHANNEL_TYPE_MSGQ) {
		if (config->mode == CONFIG_MODE_PD) {
			if (config->pd->channel_device)
				free(config->pd->channel_device);
			char *tmp = safe_strdup(config->config_file);
			config->pd->channel_device = safe_strdup(basename(tmp));
			free(tmp);
		}
	}
}

void config_print(struct config_s *config)
{
	int i, j, cp_mode;
	char tmp[64];
	struct config_pd_s *pd;

	cp_mode = config->mode == CONFIG_MODE_CP;

	printf("GLOBAL:\n");
	printf("config_file: %s\n", config->config_file);
	printf("mode: %d\n", config->mode);
	printf("conn_topology: %d\n", config->conn_topology);
	printf("num_pd: %d\n", config->num_pd);
	for (i = 0; i < config->num_pd; i++) {
		pd = config->pd + i;
		printf("\nPD-%d:\n", i);
		printf("name: '%s'\n", pd->name);
		printf("channel_speed: %d\n", pd->channel_speed);
		printf("channel_type: %d\n", pd->channel_type);
		printf("channel_device: %s\n", pd->channel_device);
		printf("address: %d\n", pd->address);
		atohstr(tmp, pd->scbk, 16);
		printf("scbk: %s\n", tmp);
		if (cp_mode)
			continue;
		printf("capabilities:\n");
		for (j = 0; j < OSDP_PD_CAP_SENTINEL; j++) {
			if (pd->cap[j].function_code == 0)
				continue;
			printf("\tFC-%02d %s -- [ %d, %d, %d ]\n", j,
			       cap_names[j], pd->cap[j].function_code,
			       pd->cap[j].compliance_level,
			       pd->cap[j].num_items);
		}
		printf("version: %d\n", pd->id.version);
		printf("model: %d\n", pd->id.model);
		printf("vendor_code: %d\n", pd->id.vendor_code);
		printf("serial_number: 0x%08x\n", pd->id.serial_number);
		printf("firmware_version: %d\n", pd->id.firmware_version);
	}
}
