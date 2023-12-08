/*
 * Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <osdp.h>
#include "test.h"

#define SEND_FILE "test-file-tx-send.txt"
#define REC_FILE "test-file-tx-receive.txt"
#define FILE_CONTENT_REPS (200)
#define FILE_CONTENT_CHUNK "0123456789abcde\n"
#define FILE_CONTENT_CHUNK_LEN (16)

struct test_data {
	bool is_cp;
	int file_id;
	int fd;
};

struct test_data sender_data;
struct test_data receiver_data;

static int test_fops_open(void *arg, int file_id, int *size)
{
	struct test_data *t = arg;

	if (file_id != 1 || t->fd != 0) {
		printf("%s_open: fd:%d send_fd:%d\n",
			t->is_cp ? "sender" : "receiver", file_id, t->fd);
		return -1;
	}

	t->file_id = file_id;

	if (t->is_cp)
		t->fd = open(SEND_FILE, O_RDONLY);
	else
		t->fd = open(REC_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

	if (t->fd < 0) {
		printf(SUB_1 "%s_open: source re-open failed\n",
			t->is_cp ? "sender" : "receiver");
		return -1;
	}

	*size = FILE_CONTENT_REPS * FILE_CONTENT_CHUNK_LEN;

	return 0;
}

static int test_fops_read(void *arg, void *buf, int size, int offset)
{
	struct test_data *t = arg;
	ssize_t ret;

	if (t->fd <= 0) {
		printf(SUB_1 "%s_read: fd:%d\n",
			t->is_cp ? "sender" : "receiver", t->fd);
		return -1;
	}

	ret = pread(t->fd, buf, (size_t)size, (size_t)offset);

	return (int)ret;
}

static int test_fops_write(void *arg, const void *buf, int size, int offset)
{
	ssize_t ret;
	struct test_data *t = arg;

	if (t->fd <= 0) {
		printf(SUB_1 "%s_write: fd:%d\n",
			t->is_cp ? "sender" : "receiver", t->fd);
		return -1;
	}

	ret = pwrite(t->fd, buf, (size_t)size, (size_t)offset);

	return (int)ret;
}

static int test_fops_close(void *arg)
{
	struct test_data *t = arg;

	if (t->fd == 0) {
		printf(SUB_1 "%s_close: fd:%d\n",
			t->is_cp ? "sender" : "receiver", t->fd);
		return -1;
	}
	close(t->fd);
	t->fd = 0;
	return 0;
}

static int test_create_file()
{
	int fd, rc, i;

	fd = open(SEND_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror(SUB_1 "sender_open: source file open failed");
		return -1;
	}

	for (i = 0; i < FILE_CONTENT_REPS; i++) {
		rc = write(fd, FILE_CONTENT_CHUNK, FILE_CONTENT_CHUNK_LEN);
		if (rc != FILE_CONTENT_CHUNK_LEN) {
			printf(SUB_1 "source file write failed at chunk%i\n", i);
			return -1;
		}
	}
	close(fd);
	return 0;
}

static bool test_check_rec_file()
{
	int i, rc, rec_fd;
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

void run_file_tx_tests(struct test *t, bool line_noise)
{
	bool result = false;
	int rc, size, offset;
	osdp_t *cp_ctx, *pd_ctx;
	int cp_runner = -1, pd_runner = -1;
	uint8_t status = 0;

	sender_data.is_cp = true;
	struct osdp_file_ops sender_ops = {
		.arg = (void *)&sender_data,
		.open = test_fops_open,
		.read = test_fops_read,
		.write = test_fops_write,
		.close = test_fops_close
	};

	struct osdp_file_ops receiver_ops = {
		.arg = (void *)&receiver_data,
		.open = test_fops_open,
		.read = test_fops_read,
		.write = test_fops_write,
		.close = test_fops_close
	};

	printf("\nBegin file transfer test\n");

	printf(SUB_1 "setting up OSDP devices\n");

	if (test_setup_devices(t, &cp_ctx, &pd_ctx)) {
		printf(SUB_1 "Failed to setup devices!\n");
		goto error;
	}

	if (test_create_file())
		goto error;

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
		osdp_get_status_mask(cp_ctx, &status);
		if (status & 1)
			break;
		usleep(1000 * 1000);
		rc++;
	}

	printf(SUB_1 "initiating file tx command\n");

	struct osdp_cmd cmd = {
		.id = OSDP_CMD_FILE_TX,
		.file_tx = {
			.id = 1,
			.flags = 0,
		}
	};
	if (osdp_cp_send_command(cp_ctx, 0, &cmd)) {
		printf(SUB_1 "Failed to initiate file tx command\n");
		goto error;
	}

	printf(SUB_1 "monitoring file tx progress\n");
	if (line_noise)
		enable_line_noise();

	while (1) {
		usleep(100 * 1000);
		rc = osdp_get_file_tx_status(cp_ctx, 0, &size, &offset);
		if (rc < 0) {
			printf(SUB_1 "status query failed!\n");
			if (line_noise)
				print_line_noise_stats();
			goto error;
		}
		if (offset == size)
			break;
	};

	result = test_check_rec_file();
	printf(SUB_1 "file transfer test %s\n",
	       result ? "succeeded" : "failed");
error:
	disable_line_noise();
	async_runner_stop(cp_runner);
	async_runner_stop(pd_runner);

	osdp_cp_teardown(cp_ctx);
	osdp_pd_teardown(pd_ctx);

	TEST_REPORT(t, result);
}
