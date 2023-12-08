/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_TEST_H_
#define _OSDP_TEST_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "osdp_common.h"

#define SUB_1 "    -- "
#define SUB_2 "        -- "

#define DO_TEST(t, m)                                                          \
	do {                                                                   \
		t->tests++;                                                    \
		if (m(t->mock_data)) {                                         \
			t->failure++;                                          \
		} else {                                                       \
			t->success++;                                          \
		}                                                              \
	} while (0)

#define TEST_REPORT(t, s)                                                      \
	do {                                                                   \
		t->tests++;                                                    \
		if (s == true)                                                 \
			t->success++;                                          \
		else                                                           \
			t->failure++;                                          \
	} while (0)

#define CHECK_ARRAY(a, l, e)                                                   \
	do {                                                                   \
		if (l < 0)                                                     \
			printf("error! invalid length %d\n", len);             \
		else if (l != sizeof(e) || memcmp(a, e, sizeof(e))) {          \
			printf("error! comparison failed!\n");                 \
			hexdump(e, sizeof(e), SUB_1 "Expected");               \
			hexdump(a, l, SUB_1 "Found");                          \
			return -1;                                             \
		}                                                              \
	} while (0)

struct test {
	int loglevel;
	int success;
	int failure;
	int tests;
	void *mock_data;
};

/* Helpers */
int test_setup_devices(struct test *t, osdp_t **cp, osdp_t **pd);
int async_runner_start(osdp_t *ctx, void (*fn)(osdp_t *));
int async_runner_stop(int runner);
void enable_line_noise();
void disable_line_noise();
void print_line_noise_stats();

void run_cp_fsm_tests(struct test *t);
void run_cp_phy_fsm_tests(struct test *t);
void run_cp_phy_tests(struct test *t);
void run_file_tx_tests(struct test *t, bool line_noise);

#endif
