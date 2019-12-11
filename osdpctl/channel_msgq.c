/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "common.h"

struct channel_msgq_s {
	int send_id;
	int send_msgid;
	int recv_id;
	int recv_msgid;
};

struct msgbuf {
	long mtype;		/* message type, must be > 0 */
	uint8_t mtext[1024];	/* message data */
} send_buf, recv_buf;

int channel_msgq_send(void *data, uint8_t *buf, int len)
{
	int ret;
	struct channel_msgq_s *ctx = data;

	send_buf.mtype = ctx->send_id;
	memcpy(&send_buf.mtext, buf, len);
	ret = msgsnd(ctx->send_msgid, &send_buf, len, 0);
	if (ret < 0 && errno == EIDRM) {
		printf("Error: msgq was removed externally. Exiting..\n");
		exit(-1);
	}

	return len;
}

int channel_msgq_recv(void *data, uint8_t *buf, int max_len)
{
	int ret;
	struct channel_msgq_s *ctx = data;

	ret = msgrcv(ctx->recv_msgid, &recv_buf, max_len,
		     ctx->recv_id, MSG_NOERROR | IPC_NOWAIT);
	if (ret == 0 || (ret < 0 && errno == EAGAIN))
		return 0;
	if (ret < 0 && errno == EIDRM) {
		printf("Error: msgq was removed externally. Exiting..\n");
		exit(-1);
	}

	if (ret > 0)
		memcpy(buf, recv_buf.mtext, ret);

	return ret;
}

void channel_msgq_flush(void *data)
{
	int ret;
	uint8_t buf[128];

	while (1) {
		ret = channel_msgq_recv(data, buf, 128);
		if (ret <= 0)
			break;
	}
}

int channel_msgq_setup(void **data, struct config_pd_s *c)
{
	key_t key;
	struct channel_msgq_s *ctx;

	ctx = malloc(sizeof(struct channel_msgq_s));
	if (ctx == NULL)
	{
		printf("Failed at alloc for uart channel\n");
		return -1;
	}

	ctx->send_id = c->is_pd_mode ? 13 : 17;
	key = ftok(c->channel_device, ctx->send_id);
	ctx->send_msgid = msgget(key, 0666 | IPC_CREAT);
	if (ctx->send_msgid < 0) {
		printf("Error: failed to create send message queue %s\n",
		       c->channel_device);
		return -1;
	}

	ctx->recv_id = c->is_pd_mode ? 17 : 13;
	key = ftok(c->channel_device, ctx->recv_id);
	ctx->recv_msgid = msgget(key, 0666 | IPC_CREAT);
	if (ctx->recv_msgid < 0) {
		printf("Error: failed to create recv message queue %s\n",
		       c->channel_device);
		return -1;
	}
	*data = (void *)ctx;

	return 0;
}

void channel_msgq_teardown(void *data)
{
	struct channel_msgq_s *ctx = data;

	msgctl(ctx->send_msgid, IPC_RMID, NULL);
	msgctl(ctx->recv_msgid, IPC_RMID, NULL);
	free(ctx);
}

struct channel_ops_s channel_msgq = {
	.send = channel_msgq_send,
	.recv = channel_msgq_recv,
	.flush = channel_msgq_flush,
	.setup = channel_msgq_setup,
	.teardown = channel_msgq_teardown
};
