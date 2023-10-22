/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include <utils/strutils.h>
#include <utils/utils.h>
#include <utils/procutils.h>
#include <utils/channel.h>

#include "common.h"

struct osdpctl_msgbuf msgq_cmd;

void pack_pd_capabilities(struct osdp_pd_cap *cap)
{
	struct osdp_pd_cap c[OSDP_PD_CAP_SENTINEL];
	int i, j = 0;

	for (i = 1; i < OSDP_PD_CAP_SENTINEL; i++) {
		if (cap[i].function_code == 0)
			continue;
		c[j].function_code = cap[i].function_code;
		c[j].compliance_level = cap[i].compliance_level;
		c[j].num_items = cap[i].num_items;
		j++;
	}
	c[j].function_code = -1;
	c[j].compliance_level = 0;
	c[j].num_items = 0;
	j++;

	memcpy(cap, c, j * sizeof(struct osdp_pd_cap));
}

int load_scbk(struct config_pd_s *c, uint8_t *buf)
{
	FILE *fd;
	char *r, hstr[33];

	fd = fopen(c->key_store, "r");
	if (fd == NULL)
		return -1;
	r = fgets(hstr, 33, fd);
	fclose(fd);
	if (r == NULL || hstrtoa(buf, hstr) != 16) {
		printf("Invalid key_store %s deleted!\n", c->key_store);
		unlink(c->key_store);
		return -1;
	}
	return 0;
}

int pd_cmd_keyset_handler(struct osdp_cmd_keyset *p)
{
	FILE *fd;
	char hstr[64];
	struct config_pd_s *c;

	c = g_config.pd;
	atohstr(hstr, p->data, p->length);
	fd = fopen(c->key_store, "w");
	if (fd == NULL) {
		perror("Error opening store_scbk file");
		return -1;
	}
	fprintf(fd, "%s\n", hstr);
	fclose(fd);

	return 0;
}

int pd_cmd_led_handler(struct osdp_cmd_led *p)
{
	hexdump((uint8_t *)p, sizeof(struct osdp_cmd_led), "PD-CMD: LED\n");
	return 0;
}

int pd_cmd_buzzer_handler(struct osdp_cmd_buzzer *p)
{
	hexdump((uint8_t *)p, sizeof(struct osdp_cmd_buzzer),
		"PD-CMD: Buzzer\n");
	return 0;
}

int pd_cmd_output_handler(struct osdp_cmd_output *p)
{
	hexdump((uint8_t *)p, sizeof(struct osdp_cmd_output),
		"PD-CMD: Output\n");
	return 0;
}

int pd_cmd_text_handler(struct osdp_cmd_text *p)
{
	hexdump((uint8_t *)p, sizeof(struct osdp_cmd_text), "PD-CMD: Text\n");
	return 0;
}

int pd_cmd_comset_handler(struct osdp_cmd_comset *p)
{
	hexdump((uint8_t *)p, sizeof(struct osdp_cmd_comset),
		"PD-CMD: ComSet\n");
	return 0;
}

int cp_event_handler(void *data, int pd, struct osdp_event *event)
{
	ARG_UNUSED(data);

	printf("CP: PD[%d]: event: %d\n", pd, event->type);
	return 0;
}

int pd_command_handler(void *data, struct osdp_cmd *cmd)
{
	ARG_UNUSED(data);

	printf("CP: CMD_ID: %d ", cmd->id);

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT:
		return pd_cmd_output_handler(&cmd->output);
	case OSDP_CMD_LED:
		return pd_cmd_led_handler(&cmd->led);
	case OSDP_CMD_BUZZER:
		return pd_cmd_buzzer_handler(&cmd->buzzer);
	case OSDP_CMD_TEXT:
		return pd_cmd_text_handler(&cmd->text);
	case OSDP_CMD_COMSET:
		return pd_cmd_comset_handler(&cmd->comset);
	case OSDP_CMD_KEYSET:
		return pd_cmd_keyset_handler(&cmd->keyset);
	default:
		break;
	}
	return -1;
}

void start_cmd_server(struct config_s *c)
{
	key_t key;

	key = ftok(c->config_file, 19);
	c->cs_send_msgid = msgget(key, 0666 | IPC_CREAT);
	if (c->cs_send_msgid < 0) {
		printf("Error: cmd server failed to create send msgq\n");
		exit(-1);
	}

	key = ftok(c->config_file, 23);
	c->cs_recv_msgid = msgget(key, 0666 | IPC_CREAT);
	if (c->cs_recv_msgid < 0) {
		printf("Error: cmd server failed to create recv msgq\n");
		exit(-1);
	}
}

void stop_cmd_server(struct config_s *c)
{
	msgctl(c->cs_send_msgid, IPC_RMID, NULL);
	msgctl(c->cs_recv_msgid, IPC_RMID, NULL);
}

void print_status(struct config_s *c)
{
	int i;
	char status = 0;
	uint8_t bitmask[16];

	printf("         \t");
	for (i = 0; i < c->num_pd; i++)
		printf("%d\t", i);
	printf("\n");

	osdp_get_status_mask(c->cp_ctx, bitmask);
	printf("   Status\t");
	for (i = 0; i < c->num_pd; i++) {
		status = (bitmask[i / 8] & (1 << (i % 8))) ? 'x' : ' ';
		printf("%c\t", status);
	}
	printf("\n");

	osdp_get_sc_status_mask(c->cp_ctx, bitmask);
	printf("SC Status\t");
	for (i = 0; i < c->num_pd; i++) {
		status = (bitmask[i / 8] & (1 << (i % 8))) ? 'x' : ' ';
		printf("%c\t", status);
	}
	printf("\n");
}

void handle_cp_command(struct config_s *c, struct osdpctl_cmd *p)
{
	if (p->id < OSDPCTL_CP_CMD_LED || p->id >= OSDPCTL_CP_CMD_SENTINEL) {
		printf("Error: got invalid command ID\n");
		return;
	}

	switch (p->id) {
	case OSDPCTL_CP_CMD_LED:
		p->cmd.id = OSDP_CMD_LED;
		break;
	case OSDPCTL_CP_CMD_BUZZER:
		p->cmd.id = OSDP_CMD_BUZZER;
		break;
	case OSDPCTL_CP_CMD_TEXT:
		p->cmd.id = OSDP_CMD_TEXT;
		break;
	case OSDPCTL_CP_CMD_OUTPUT:
		p->cmd.id = OSDP_CMD_OUTPUT;
		break;
	case OSDPCTL_CP_CMD_KEYSET:
		p->cmd.id = OSDP_CMD_KEYSET;
		break;
	case OSDPCTL_CP_CMD_COMSET:
		p->cmd.id = OSDP_CMD_COMSET;
		break;
	case OSDPCTL_CMD_STATUS:
		print_status(c);
		return;
	}

	osdp_cp_send_command(c->cp_ctx, p->offset, &p->cmd);
}

int process_commands(struct config_s *c)
{
	int ret;

	ret = msgrcv(c->cs_recv_msgid, &msgq_cmd, 1024, 1, IPC_NOWAIT);
	if (ret == 0 || (ret < 0 && errno == EAGAIN))
		return 0;
	if (ret < 0 && errno == EIDRM) {
		printf("Error: msgq was removed externally. Exiting..\n");
		exit(-1);
	}
	if (ret <= 0)
		return -1;
	;

	if (c->mode == CONFIG_MODE_CP)
		handle_cp_command(c, (struct osdpctl_cmd *)msgq_cmd.mtext);

	return 0;
}

int cmd_handler_start(int argc, char *argv[], void *data)
{
	int i, ret;
	osdp_pd_info_t *info_arr, *info;
	struct config_pd_s *pd;
	uint8_t *scbk, scbk_buf[16];
	struct config_s *c = data;

	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	if (c->log_file) {
		printf("Redirecting stdout and stderr to log_file %s\n",
		       c->log_file);
		o_redirect(3, c->log_file);
	}

	if (read_pid(c->pid_file, NULL) == 0) {
		printf("Error: A service for this file already exists!\n"
		       "If you are sure it doesn't, remove %s and retry.\n",
		       c->pid_file);
		return -1;
	}

	start_cmd_server(c);
	write_pid(c->pid_file);
	c->service_started = 1;

	info_arr = calloc(c->num_pd, sizeof(osdp_pd_info_t));
	if (info_arr == NULL) {
		printf("Failed to alloc info struct\n");
		return -1;
	}

	for (i = 0; i < c->num_pd; i++) {
		info = info_arr + i;
		pd = c->pd + i;
		info->name = pd->name;
		info->address = pd->address;
		info->baud_rate = pd->channel_speed;

		ret = channel_open(&c->chn_mgr, pd->channel_type,
				   pd->channel_device, pd->channel_speed,
				   pd->is_pd_mode);
		if (ret != CHANNEL_ERR_NONE &&
		    ret != CHANNEL_ERR_ALREADY_OPEN) {
			printf("Failed to setup channel\n");
			exit(-1);
		}

		channel_get(&c->chn_mgr, pd->channel_device, &info->channel.id,
			    &info->channel.data, &info->channel.send,
			    &info->channel.recv, &info->channel.flush);

		if (c->mode == CONFIG_MODE_CP)
			continue;

		memcpy(&info->id, &pd->id, sizeof(struct osdp_pd_id));
		pack_pd_capabilities(pd->cap);
		info->cap = pd->cap;
		info->scbk = NULL;
	}

	osdp_logger_init3("osdp::cp", c->log_level, NULL);

	if (c->mode == CONFIG_MODE_CP) {
		c->cp_ctx = osdp_cp_setup2(c->num_pd, info_arr);
		if (c->cp_ctx == NULL) {
			printf("Failed to setup CP context\n");
			return -1;
		}
		osdp_cp_set_event_callback(c->cp_ctx, cp_event_handler, NULL);
	} else {
		scbk = NULL;
		if (load_scbk(c->pd, scbk_buf) == 0)
			scbk = scbk_buf;
		info_arr->scbk = scbk;
		c->pd_ctx = osdp_pd_setup(info_arr);
		if (c->pd_ctx == NULL) {
			printf("Failed to setup PD context\n");
			return -1;
		}
		osdp_pd_set_command_callback(c->pd_ctx, pd_command_handler, NULL);
	}

	free(info_arr);

	while (1) {
		if (c->mode == CONFIG_MODE_CP) {
			osdp_cp_refresh(c->cp_ctx);
		} else {
			osdp_pd_refresh(c->pd_ctx);
		}
		process_commands(c);
		usleep(20 * 1000);
	}

	return 0;
}
