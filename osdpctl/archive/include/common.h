/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDPCTL_COMMON_H_
#define _OSDPCTL_COMMON_H_

#include <stdint.h>
#include <osdp.h>
#include <utils/utils.h>
#include <utils/channel.h>

#define CONFIG_FILE_PATH_LENGTH 128
#define ARG_UNUSED(x)		(void)(x)

enum config_osdp_mode_e { CONFIG_MODE_CP = 1, CONFIG_MODE_PD };

enum config_channel_topology_e {
	CONFIG_CHANNEL_TOPOLOGY_CHAIN = 1,
	CONFIG_CHANNEL_TOPOLOGY_STAR
};

struct config_pd_s {
	char *name;
	char *channel_device;
	enum channel_type channel_type;
	int channel_speed;

	int address;
	int is_pd_mode;
	char *key_store;

	struct osdp_pd_id id;
	struct osdp_pd_cap cap[OSDP_PD_CAP_SENTINEL];
	uint8_t scbk[16];
};

struct config_s {
	/* ini section: "^GLOBAL" */
	int mode;
	int num_pd;
	int log_level;
	int conn_topology;

	/* ini section: "^PD(-[0-9]+)?" */
	struct config_pd_s *pd;

	struct channel_manager chn_mgr;

	osdp_t *cp_ctx;
	osdp_t *pd_ctx;

	int service_started;
	int cs_send_msgid;
	int cs_recv_msgid;

	/* cli_args */
	char *pid_file;
	char *log_file;
	char *config_file;
};

extern struct config_s g_config;

void config_print(struct config_s *config);
void config_parse(const char *filename, struct config_s *config);

// command handlers

void stop_cmd_server(struct config_s *c);
int cmd_handler_start(int argc, char *argv[], void *data);
int cmd_handler_send(int argc, char *argv[], void *data);
int cmd_handler_stop(int argc, char *argv[], void *data);
int cmd_handler_check(int argc, char *argv[], void *data);

// API

enum osdpctl_cmd_e {
	OSDPCTL_CMD_UNUSED,
	OSDPCTL_CP_CMD_LED,
	OSDPCTL_CP_CMD_BUZZER,
	OSDPCTL_CP_CMD_TEXT,
	OSDPCTL_CP_CMD_OUTPUT,
	OSDPCTL_CP_CMD_COMSET,
	OSDPCTL_CP_CMD_KEYSET,
	OSDPCTL_CMD_STATUS,
	OSDPCTL_CP_CMD_SENTINEL
};

struct osdpctl_cmd {
	int id;
	int offset;
	struct osdp_cmd cmd;
};

struct osdpctl_msgbuf {
	long mtype; /* message type, must be > 0 */
	uint8_t mtext[1024]; /* message data */
};

extern struct osdpctl_msgbuf msgq_cmd;

#endif
