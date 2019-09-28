/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
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

	test_start(&t);

	run_cp_phy_tests(&t);

	run_cp_phy_fsm_tests(&t);

	run_cp_fsm_tests(&t);

	run_mixed_fsm_tests(&t);

	return test_end(&t);
}
