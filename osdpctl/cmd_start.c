/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
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

#include "common.h"

struct msgbuf msgq_cmd;

void pack_pd_capabilities(struct pd_cap *cap)
{
	struct pd_cap c[CAP_SENTINEL];
	int i, j = 0;

	for (i = 1; i < CAP_SENTINEL; i++) {
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

	memcpy(cap, c, j * sizeof(struct pd_cap));
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
	if (r == NULL || hstrtoa(buf, hstr))
		return -1;

	return 0;
}

int pd_cmd_keyset_handler(struct osdp_cmd_keyset *p)
{
	FILE *fd;
	char hstr[64];
	struct config_pd_s *c;

	c = g_config.pd;
	atohstr(hstr, p->data, p->len);
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
	osdp_dump("PD-CMD: LED\n", (uint8_t *)p, sizeof(struct osdp_cmd_led));
	return 0;
}

int pd_cmd_buzzer_handler(struct osdp_cmd_buzzer *p)
{
	osdp_dump("PD-CMD: Buzzer\n", (uint8_t *)p, sizeof(struct osdp_cmd_buzzer));
	return 0;
}

int pd_cmd_output_handler(struct osdp_cmd_output *p)
{
	osdp_dump("PD-CMD: Output\n", (uint8_t *)p, sizeof(struct osdp_cmd_output));
	return 0;
}

int pd_cmd_text_handler(struct osdp_cmd_text *p)
{
	osdp_dump("PD-CMD: Text\n", (uint8_t *)p, sizeof(struct osdp_cmd_text));
	return 0;
}

int pd_cmd_comset_handler(struct osdp_cmd_comset *p)
{
	osdp_dump("PD-CMD: ComSet\n", (uint8_t *)p, sizeof(struct osdp_cmd_comset));
	return 0;
}

int cp_keypress_handler(int pd, uint8_t key)
{
	printf("CP: PD[%d]: keypressed: 0x%02x\n", pd, key);
	return 0;
}

int cp_card_read_handler(int pd, int format, uint8_t * data, int len)
{
	int i;

	printf("CP: PD[%d]: cardRead: FMT: %d Data[%d]: { ", pd, format, len);
	for (i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("}\n");

	return 0;
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

void handle_cp_command(struct config_s *c, struct osdpctl_cmd *p)
{
	if (p->id < OSDPCTL_CP_CMD_LED || p->id >= OSDPCTL_CP_CMD_SENTINEL) {
		printf("Error: got invalid command ID\n");
		return;
	}

	switch(p->id) {
	case OSDPCTL_CP_CMD_LED:
		osdp_cp_send_cmd_led(c->cp_ctx, p->offset, &p->cmd.led);
		break;
	case OSDPCTL_CP_CMD_BUZZER:
		osdp_cp_send_cmd_buzzer(c->cp_ctx, p->offset, &p->cmd.buzzer);
		break;
	case OSDPCTL_CP_CMD_TEXT:
		osdp_cp_send_cmd_text(c->cp_ctx, p->offset, &p->cmd.text);
		break;
	case OSDPCTL_CP_CMD_OUTPUT:
		osdp_cp_send_cmd_output(c->cp_ctx, p->offset, &p->cmd.output);
		break;
	case OSDPCTL_CP_CMD_KEYSET:
		osdp_cp_send_cmd_keyset(c->cp_ctx, &p->cmd.keyset);
		break;
	case OSDPCTL_CP_CMD_COMSET:
		osdp_cp_send_cmd_comset(c->cp_ctx, p->offset, &p->cmd.comset);
		break;
	}
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
	if (ret <= 0) return -1;;

	if (c->mode == CONFIG_MODE_CP)
		handle_cp_command(c, (struct osdpctl_cmd *)msgq_cmd.mtext);

	return 0;
}

int cmd_handler_start(int argc, char *argv[], void *data)
{
	int i;
	osdp_pd_info_t *info_arr, *info;
	struct config_pd_s *pd;
	uint8_t *scbk, scbk_buf[16];
	struct config_s *c = data;

	if (argc < 1) {
		printf ("Error: must pass a config file\n");
		return -1;
	}
	config_parse(argv[0], c);
	if (c->log_file) {
		printf("Redirecting output to log_file %s\n", c->log_file);
		redirect_output_to_log_file(c->log_file);
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
		info->address = pd->address;
		info->baud_rate = pd->channel_speed;
		if (channel_setup(pd)) {
			printf("Failed to setup channel\n");
			exit (-1);
		}
		if (pd->channel.flush)
			pd->channel.flush(pd->channel.data);
		memcpy(&info->channel, &pd->channel, sizeof(struct osdp_channel));

		if (c->mode == CONFIG_MODE_CP)
			continue;

		memcpy(&info->id, &pd->id, sizeof(struct pd_id));
		pack_pd_capabilities(pd->cap);
		info->cap = pd->cap;
	}

	osdp_set_log_level(c->log_level);

	if (c->mode == CONFIG_MODE_CP) {
		c->cp_ctx = osdp_cp_setup(c->num_pd, info_arr, c->cp.master_key);
		if (c->cp_ctx == NULL) {
			printf("Failed to setup CP context\n");
			return -1;
		}
		osdp_cp_set_callback_key_press(c->cp_ctx, cp_keypress_handler);
		osdp_cp_set_callback_card_read(c->cp_ctx, cp_card_read_handler);
	} else {
		scbk = NULL;
		if (load_scbk(c->pd, scbk_buf) == 0)
			scbk = scbk_buf;
		c->pd_ctx = osdp_pd_setup(info_arr, scbk);
		if (c->pd_ctx == NULL) {
			printf("Failed to setup PD context\n");
			return -1;
		}
		osdp_pd_set_callback_cmd_led(c->pd_ctx, pd_cmd_led_handler);
		osdp_pd_set_callback_cmd_buzzer(c->pd_ctx, pd_cmd_buzzer_handler);
		osdp_pd_set_callback_cmd_output(c->pd_ctx, pd_cmd_output_handler);
		osdp_pd_set_callback_cmd_text(c->pd_ctx, pd_cmd_text_handler);
		osdp_pd_set_callback_cmd_comset(c->pd_ctx, pd_cmd_comset_handler);
		osdp_pd_set_callback_cmd_keyset(c->pd_ctx, pd_cmd_keyset_handler);
	}

	free(info_arr);

	while (1) {
		if (c->mode == CONFIG_MODE_CP)
			osdp_cp_refresh(c->cp_ctx);
		else
			osdp_pd_refresh(c->pd_ctx);
		process_commands(c);
		usleep(20 * 1000);
	}

	return 0;
}
