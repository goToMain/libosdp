/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ini.h"
#include "common.h"

#define INI_SUCCESS 1
#define INI_FAILURE 0

void osdp_dump(const char *head, const uint8_t *data, int len);

int config_parse_key_mode(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp(val, "CP") != 0)
		p->osdp_mode = CONFIG_OSDP_MODE_CP;
	else if (strcmp(val, "PD") != 0)
		p->osdp_mode = CONFIG_OSDP_MODE_PD;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

int config_parse_key_channel_topology(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp(val, "chain") == 0)
		p->channel_topology = CONFIG_CHANNEL_TOPOLOGY_CHAIN;
	else if (strcmp(val, "star") == 0)
		p->channel_topology = CONFIG_CHANNEL_TOPOLOGY_STAR;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

int config_parse_key_channel_type(const char *val, void *data)
{
	struct config_s *p = data;

	if (strcmp(val, "uart") == 0)
		p->channel_type = CONFIG_CHANNEL_TYPE_UART;
	else if (strcmp(val, "unix") == 0)
		p->channel_type = CONFIG_CHANNEL_TYPE_UNIX;
	else if (strcmp(val, "internal") == 0)
		p->channel_type = CONFIG_CHANNEL_TYPE_INTERNAL;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

int config_parse_key_channel_speed(const char *val, void *data)
{
	struct config_s *p = data;
	int baud = atoi(val);

	if (baud != 9600 && baud != 38400 && baud != 115200) {
		printf("Invalid baudrate %d\n", baud);
		return INI_FAILURE;
	}
	p->channel_speed = baud;

	return INI_SUCCESS;
}

int config_parse_key_channel_device(const char *val, void *data)
{
	struct config_s *p = data;

	if (access(val, F_OK) == -1) {
		printf("device %s does not exist\n", val);
		return INI_FAILURE;
	}
	p->channel_device = strdup(val);

	return INI_SUCCESS;
}

int config_parse_key_num_pd(const char *val, void *data)
{
	struct config_s *p = data;
	int num_pd = atoi(val);

	if (num_pd == 0)
		return INI_FAILURE;
	p->num_pd = num_pd;

	return INI_SUCCESS;
}

int config_parse_key_master_key(const char* val, void *data)
{
	uint8_t tmp[128];
	struct config_s *p = data;

	if (hstrtoa(tmp, val) < 0) {
		printf("Failed to parse master key\n");
		return INI_FAILURE;
	}
	memcpy(p->master_key, tmp, 16);

	return INI_SUCCESS;
}

int config_parse_key_address(const char *val, void *data)
{
	struct config_s *p = data;
	int addr = atoi(val);

	if (addr == 0)
		return INI_FAILURE;
	p->pd_address = addr;

	return INI_SUCCESS;
}

int config_parse_key_vendor_code(const char *val, void *data)
{
	struct config_s *p = data;
	int i = atoi(val);

	if (i == 0)
		return INI_FAILURE;
	p->vendor_code = i;

	return INI_SUCCESS;
}

int config_parse_key_model(const char *val, void *data)
{
	struct config_s *p = data;
	int i = atoi(val);

	if (i == 0)
		return INI_FAILURE;
	p->model = i;

	return INI_SUCCESS;
}

int config_parse_key_version(const char *val, void *data)
{
	struct config_s *p = data;
	int i = atoi(val);

	if (i == 0)
		return INI_FAILURE;
	p->version = i;

	return INI_SUCCESS;
}

int config_parse_serial_number(const char *val, void *data)
{
	struct config_s *p = data;
	int i = atoi(val);

	if (i == 0)
		return INI_FAILURE;
	p->serial_number = i;

	return INI_SUCCESS;
}

struct config_key_s {
	const char *key;
	int (*handler)(const char *val, void *data);
};

const struct config_key_s g_config_key_global[] = {
	{ "mode",		config_parse_key_mode },
	{ "channel_topology",	config_parse_key_channel_topology },
	{ "channel_type",	config_parse_key_channel_type },
	{ "channel_speed",	config_parse_key_channel_speed },
	{ "channel_device",	config_parse_key_channel_device },
	{ NULL, NULL }
};

const struct config_key_s g_config_key_cp[] = {
	{ "num_pd",		config_parse_key_num_pd },
	{ "master_key",		config_parse_key_master_key },
	{ NULL, NULL }
};

const struct config_key_s g_config_key_pd[] = {
	{ "address", 		config_parse_key_address },
	{ "vendor_code",	config_parse_key_vendor_code },
	{ "model",		config_parse_key_model },
	{ "version",		config_parse_key_version },
	{ "serial_number",	config_parse_serial_number },
	{ NULL, NULL }
};

int config_key_parse(const char *key, const char*val,
		     const struct config_key_s *p, void *data)
{
	while(p && p->key) {
		if (strcmp(p->key, key) == 0 && p->handler) {
			return p->handler(val, data);
		}
		p++;
	}

	return INI_FAILURE;
}

int config_ini_cb(void* data, const char *sec, const char *key, const char *val)
{
	if (strcmp("GLOBAL", sec) == 0)
		return config_key_parse(key, val, g_config_key_global, data);
	if (strcmp("CP", sec) == 0)
		return config_key_parse(key, val, g_config_key_cp, data);
	if (strcmp("PD", sec) == 0)
		return config_key_parse(key, val, g_config_key_pd, data);

	return INI_FAILURE;
}

void config_parse(const char *filename, struct config_s *config)
{
	int ret = ini_parse(filename, config_ini_cb, config);

	if (ret == -1) {
		printf("Error: unable to open file: %s\n", filename);
		exit(-1);
	}
	if (ret == -2) {
		printf("Error: memory alloc failed when paring: %s\n", filename);
		exit(-1);
	}
	if (ret < 0) {
		printf("Error: in file: %s\n", filename);
		exit(-1);
	}
	if (ret > 0) {
		printf("Error parsing file %s at line %d", filename, ret);
		exit(-1);
	}
}

void config_print(struct config_s *config)
{
	char tmp[64];

	printf("\n--- BEGIN (%s) ---\n\n", config->config_file);

	printf("GLOBAL:\n");
	printf("mode: %d\n", config->osdp_mode);
	printf("channel_speed: %d\n", config->channel_speed);
	printf("channel_type: %d\n", config->channel_type);
	printf("channel_topology: %d\n", config->channel_topology);
	printf("channel_device: %s\n", config->channel_device);

	printf("\nCP:\n");
	printf("num_pd: %d\n", config->num_pd);
	atohstr(tmp, config->master_key, 16);
	printf("master_key: %s\n", tmp);

	printf("\nPD:\n");
	printf("address: %d\n", config->pd_address);
	printf("vendor_code: %d\n", config->vendor_code);
	printf("model: %d\n", config->model);
	printf("version: %d\n", config->version);
	printf("serial_number: 0x%08x\n", config->serial_number);

	printf("\n--- END ---\n\n");
}
