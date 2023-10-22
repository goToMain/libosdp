/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <osdp.h>

#include "osdp_common.h"
#include "test.h"

#include <utils/workqueue.h>
#include <utils/circbuf.h>

#define MAX_TEST_WORK 5
#define MOCK_BUF_LEN 512

CIRCBUF_DEF(uint8_t, cp_to_pd_buf, MOCK_BUF_LEN);
CIRCBUF_DEF(uint8_t, pd_to_cp_buf, MOCK_BUF_LEN);

struct test_async_data {
	osdp_t *ctx;
	void (*refresh)(osdp_t *ctx);
	struct async_runner_s *runner;
};

workqueue_t test_wq;

int async_runner(void *data)
{
	struct test_async_data *td = data;

	td->refresh(td->ctx);
	usleep(10 * 1000);

	return WORK_YIELD; // will be cancelled if needed.
}

void async_runner_free(void *data)
{
	work_t *work = data;

	free(work->arg);
	free(work);
}

work_t *g_test_works[MAX_TEST_WORK];

int async_runner_start(osdp_t *ctx, void (*fn)(osdp_t *))
{
	int i, rc;

	struct test_async_data *data = malloc(sizeof(struct test_async_data));
	data->ctx = ctx;
	data->refresh = fn;

	for (i = 0; i < MAX_TEST_WORK; i++) {
		if (g_test_works[i] == NULL)
			break;
	}

	if (i == MAX_TEST_WORK) {
		printf("async_runner_start: test works exhausted\n");
		exit(EXIT_FAILURE);
	}

	g_test_works[i] = malloc(sizeof(work_t));
	g_test_works[i]->arg = data;
	g_test_works[i]->work_fn = async_runner;
	g_test_works[i]->complete_fn = NULL;

	rc = workqueue_add_work(&test_wq, g_test_works[i]);
	if (rc != 0) {
		printf("async_runner_start: test wq add work failed!\n");
		exit(EXIT_FAILURE);
	}

	return i;
}

int async_runner_stop(int work_id)
{
	if (work_id < 0 || work_id > MAX_TEST_WORK || g_test_works[work_id]== NULL) {
		printf("async_runner_stop: invalid work id!\n");
		exit(EXIT_FAILURE);
	}

	workqueue_cancel_work(&test_wq, g_test_works[work_id]);
	while (workqueue_work_is_complete(&test_wq, g_test_works[work_id]) == false)
		usleep(50 * 1000);
	async_runner_free(g_test_works[work_id]);
	g_test_works[work_id] = NULL;

	return 0;
}

volatile bool g_introduce_line_noise;
unsigned long g_total_packets;
unsigned long g_corrupted_packets;

void enable_line_noise()
{
	g_introduce_line_noise = true;
}

void disable_line_noise()
{
	g_introduce_line_noise = false;
}

void print_line_noise_stats()
{
	printf(SUB_1 "LN-Stats: Total:%lu Corrupted:%lu",
	       g_total_packets, g_corrupted_packets);
}

void corrupt_buffer(uint8_t *buf, int len)
{
	int p, n = 3;

	while (n--) {
		p = randint(len);
		buf[p] = randint(255);
	}
}

void maybe_corrupt_buffer(uint8_t *buf, int len)
{
	if (!g_introduce_line_noise)
		return;
	g_total_packets++;
	if (randint(10*1000) < (5*1000))
		return;
	corrupt_buffer(buf, len);
	g_corrupted_packets++;
}

int test_mock_cp_send(void *data, uint8_t *buf, int len)
{
	int i;
	ARG_UNUSED(data);
	assert(len < MOCK_BUF_LEN);

	maybe_corrupt_buffer(buf, len);
	for (i = 0; i < len; i++) {
		if (CIRCBUF_PUSH(cp_to_pd_buf, buf + i))
			break;
	}
	return i;
}

int test_mock_cp_receive(void *data, uint8_t *buf, int len)
{
	int i;
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	for (i = 0; i < len; i++) {
		if (CIRCBUF_POP(pd_to_cp_buf, buf + i))
			break;
	}
	return i;
}

int test_mock_pd_send(void *data, uint8_t *buf, int len)
{
	int i;
	ARG_UNUSED(data);

	maybe_corrupt_buffer(buf, len);
	for (i = 0; i < len; i++) {
		if (CIRCBUF_PUSH(pd_to_cp_buf, buf + i))
			break;
	}
	return i;
}

int test_mock_pd_receive(void *data, uint8_t *buf, int len)
{
	int i;
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	for (i = 0; i < len; i++) {
		if (CIRCBUF_POP(cp_to_pd_buf, buf + i))
			break;
	}
	return i;
}

int test_setup_devices(struct test *t, osdp_t **cp, osdp_t **pd)
{
	osdp_logger_init3("osdp", t->loglevel, NULL);

	uint8_t scbk[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	/* mock application data */
	osdp_pd_info_t info_cp = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0, //OSDP_FLAG_ENFORCE_SECURE,
		.channel.data = NULL,
		.channel.send = test_mock_cp_send,
		.channel.recv = test_mock_cp_receive,
		.channel.flush = NULL,
		.scbk = scbk,
	};

	*cp = osdp_cp_setup2(1, &info_cp);
	if (*cp == NULL) {
		printf("   cp init failed!\n");
		return -1;
	}

	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 9600,
		.flags = 0, //OSDP_FLAG_ENFORCE_SECURE,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = NULL,
		.channel.data = NULL,
		.channel.send = test_mock_pd_send,
		.channel.recv = test_mock_pd_receive,
		.scbk = scbk,
	};

	*pd = (struct osdp *)osdp_pd_setup(&info_pd);
	if (*pd == NULL) {
		printf(SUB_1 "pd init failed!\n");
		osdp_cp_teardown(*cp);
		return -1;
	}

	return 0;
}

void test_start(struct test *t, int log_level)
{
	printf("\n");
	printf("------------------------------------------\n");
	printf("            OSDP - Unit Tests             \n");
	printf("------------------------------------------\n");

	t->tests = 0;
	t->success = 0;
	t->failure = 0;
	t->loglevel = log_level;
}

int test_end(struct test *t)
{
	printf("\n");
	printf("------------------------------------------\n");
	printf("Tests: %d\tSuccess: %d\tFailure: %d\n", t->tests, t->success,
	       t->failure);
	printf("\n");

	if (t->tests != t->success)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;
	struct test t;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	srand(time(NULL));

	workqueue_create(&test_wq, MAX_TEST_WORK);

	test_start(&t, OSDP_LOG_INFO);

	run_cp_phy_tests(&t);

	run_cp_phy_fsm_tests(&t);

	run_cp_fsm_tests(&t);

	run_file_tx_tests(&t, false);

	rc = test_end(&t);

	workqueue_destroy(&test_wq);

	return rc;
}
