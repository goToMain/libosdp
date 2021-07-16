/*
 * Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <osdp.h>
#include "test.h"

int send_fd, rec_fd;
#define SEND_FILE "test-file-tx-send.txt"
#define REC_FILE "test-file-tx-receive.txt"
#define FILE_CONTENT_REPS (200)
#define FILE_CONTENT_CHUNK "0123456789abcde\n"
#define FILE_CONTENT_CHUNK_LEN (16)

int sender_open(int fd, int *size)
{
	int i, rc;

	if (fd != 1 || send_fd != 0) {
		printf("sender_open: fd:%d send_fd:%d\n", fd, send_fd);
		return -1;
	}

	send_fd = open(SEND_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (send_fd < 0) {
		perror(SUB_1 "sender_open: source file open failed");
		return -1;
	}

	for (i = 0; i < FILE_CONTENT_REPS; i++) {
		rc = write(send_fd, FILE_CONTENT_CHUNK,
				FILE_CONTENT_CHUNK_LEN);
		if (rc != FILE_CONTENT_CHUNK_LEN) {
			printf(SUB_1 "sender_open: source file write "
				"failed at chunk %i\n", i);
			return -1;
		}
	}
	close(send_fd);

	send_fd = open(SEND_FILE, O_RDONLY);
	if (send_fd < 0) {
		printf(SUB_1 "sender_open: source re-open failed\n");
		return -1;
	}

	*size = FILE_CONTENT_REPS * FILE_CONTENT_CHUNK_LEN;

	return 0;
}

int sender_read(int fd, void *buf, int size, int offset)
{
	ssize_t ret;
	if (fd != 1 || send_fd <= 0) {
		printf(SUB_1 "sender_read: fd:%d send_fd:%d\n", fd, send_fd);
		return -1;
	}

	ret = pread(send_fd, buf, (size_t)size, (size_t)offset);

	return (int)ret;
}

int sender_write(int fd, const void *buf, int size, int offset)
{
	(void)fd;
	(void)buf;
	(void)size;
	(void)offset;
	return -1; /* sender cannot write */
}

void sender_close(int fd)
{
	if (fd != 1 || send_fd == 0) {
		printf(SUB_1 "sender_close: fd:%d send_fd:%d\n", fd, send_fd);
		return;
	}

	close(send_fd);
	send_fd = 0;
}

struct osdp_file_ops sender_ops = {
	.open = sender_open,
	.read = sender_read,
	.write = sender_write,
	.close = sender_close
};

int receiver_open(int fd, int *size)
{
	if (fd != 1 || rec_fd != 0) {
		printf(SUB_1 "receiver_open: fd:%d send_fd:%d\n", fd, rec_fd);
		return -1;
	}

	(void)size;
	rec_fd = open(REC_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (rec_fd < 0) {
		perror(SUB_1 "receiver_open: failed\n");
		return -1;
	}

	return 0;
}

int receiver_read(int fd, void *buf, int size, int offset)
{
	(void)fd;
	(void)buf;
	(void)size;
	(void)offset;
	return -1; /* receiver cannot write */
}

int receiver_write(int fd, const void *buf, int size, int offset)
{
	ssize_t ret;
	if (fd != 1 || rec_fd <= 0) {
		printf(SUB_1 "receiver_write: fd:%d send_fd:%d\n", fd, rec_fd);
		return -1;
	}

	ret = pwrite(rec_fd, buf, (size_t)size, (size_t)offset);

	return (int)ret;
}

void receiver_close(int fd)
{
	if (fd != 1 || rec_fd == 0) {
		printf(SUB_1 "receiver_close: fd:%d send_fd:%d\n", fd, rec_fd);
		return;
	}

	close(rec_fd);
	rec_fd = 0;
}

struct osdp_file_ops receiver_ops = {
	.open = receiver_open,
	.read = receiver_read,
	.write = receiver_write,
	.close = receiver_close
};

bool check_rec_file()
{
	int i, rc;
	char buf[1024];

	rec_fd = open(REC_FILE, O_RDONLY);
	if (rec_fd < 0) {
		printf(SUB_1 "check_rec_file: open rec file failed\n");
		return false;
	}

	for (i = 0; i < FILE_CONTENT_REPS; i++) {
		rc = read(rec_fd, buf, FILE_CONTENT_CHUNK_LEN);
		if (rc != FILE_CONTENT_CHUNK_LEN) {
			printf(SUB_1 "check_rec_file: rec file read "
			       "failed at chunk %i; read: %d\n", i, rc);
			goto err;
		}
		buf[rc] = 0;
		if (memcmp(buf, FILE_CONTENT_CHUNK, FILE_CONTENT_CHUNK_LEN)) {
			printf(SUB_1 "check_rec_file: memcmp failed at chunk "
			       "%d;\n" SUB_1 "got: %s", i, buf);
			goto err;
		}
	}

	rc = read(rec_fd, buf, FILE_CONTENT_CHUNK_LEN);
	if (rc != 0)
		goto err;

	close(rec_fd);

	unlink(SEND_FILE);
	unlink(REC_FILE);

	return true;
err:
	close(rec_fd);
	return false;
}

void run_file_tx_tests(struct test *t)
{
	bool result = false;
	int rc, size, offset;
	osdp_t *cp_ctx, *pd_ctx;
	int cp_runner = -1, pd_runner = -1;

	printf("\nBegin file transfer test\n");

	printf(SUB_1 "setting up OSDP devices\n");

	if (test_setup_devices(t, &cp_ctx, &pd_ctx)) {
		printf(SUB_1 "Failed to setup devices!\n");
		goto error;
	}

	osdp_file_register_ops(cp_ctx, 0, &sender_ops);
	osdp_file_register_ops(pd_ctx, 0, &receiver_ops);

	printf(SUB_1 "starting async runners\n");

	cp_runner = async_runner_start(cp_ctx, osdp_cp_refresh);
	pd_runner = async_runner_start(pd_ctx, osdp_pd_refresh);

	if (cp_runner < 0 || pd_runner < 0) {
		printf(SUB_1 "Failed to created CP/PD runners\n");
		goto error;
	}

	rc = 0;
	while (1) {
		if (rc > 10) {
			printf(SUB_1 "PD failed to come online");
			goto error;
		}
		if (osdp_get_status_mask(cp_ctx) == 1)
			break;
		usleep(1000 * 1000);
		rc++;
	}

	printf(SUB_1 "initiating file tx command\n");

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_FILE_TX,
		.file_tx = {
			.fd = 1,
			.flags = 0,
		}
	};
	if (osdp_cp_send_command(cp_ctx, 0, &cmd)) {
		printf(SUB_1 "Failed to inittate file tx command\n");
		goto error;
	}

	printf(SUB_1 "monitoring file tx progress\n");

	while (1) {
		usleep(100 * 1000);
		rc = osdp_file_tx_status(cp_ctx, 0, &size, &offset);
		if (rc < 0) {
			printf(SUB_1 "status query failed!\n");
			goto error;
		}
		if (offset == size)
			break;
	};

	result = check_rec_file();
error:
	async_runner_stop(cp_runner);
	async_runner_stop(pd_runner);


	osdp_cp_teardown(cp_ctx);
	osdp_pd_teardown(pd_ctx);

	TEST_REPORT(t, result);
}
