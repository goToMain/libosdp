/*
 * Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <osdp.h>

#include "osdp_common.h"
#include "test.h"

void test_start(struct test *t)
{
	printf("\n");
	printf("------------------------------------------\n");
	printf("            OSDP - Unit Tests             \n");
	printf("------------------------------------------\n");
	printf("\n");

	t->tests = 0;
	t->success = 0;
	t->failure = 0;
	t->loglevel = LOG_INFO;
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
	struct test t;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	srand(time(NULL));

	test_start(&t);

	run_cp_phy_tests(&t);

	run_cp_phy_fsm_tests(&t);

	run_cp_fsm_tests(&t);

	run_mixed_fsm_tests(&t);

	run_osdp_commands_tests(&t);

	return test_end(&t);
}
