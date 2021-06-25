/*
 * Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <osdp.h>
#include "test.h"

extern int (*test_state_update)(struct osdp_pd *);
extern void (*test_osdp_pd_update)(struct osdp_pd *pd);

struct test_mixed {
	struct osdp *cp_ctx;
	struct osdp *pd_ctx;
} test_data;

int test_mixed_fsm_setup(struct test *t)
{
	t->mock_data = &test_data;
	return test_setup_devices(t, (osdp_t **)&test_data.cp_ctx,
				  (osdp_t **)&test_data.pd_ctx);
}

void test_mixed_fsm_teardown(struct test *t)
{
	struct test_mixed *p = t->mock_data;

	osdp_cp_teardown((osdp_t *)p->cp_ctx);
	osdp_pd_teardown((osdp_t *)p->pd_ctx);
}

void run_mixed_fsm_tests(struct test *t)
{
	int result = true;
	struct test_mixed *p;
	struct osdp_pd *pd_cp, *pd_pd;
	int64_t start;

	printf("\nBegin CP - PD phy layer mixed tests\n");

	printf(SUB_1 "setting up OSDP devices\n");

	if (test_mixed_fsm_setup(t))
		return;

	p = t->mock_data;

	printf(SUB_1 "executing CP - PD mixed tests\n");
	start = osdp_millis_now();
	pd_cp = GET_CURRENT_PD(p->cp_ctx);
	pd_pd = GET_CURRENT_PD(p->pd_ctx);
	while (1) {
		test_state_update(pd_cp);
		test_osdp_pd_update(pd_pd);
		if (osdp_get_sc_status_mask(p->cp_ctx))
			break;
		if (pd_cp->state == OSDP_CP_STATE_OFFLINE) {
			printf(SUB_1 "CP went offline!\n");
			result = false;
			break;
		}
		if (pd_pd->state == OSDP_PD_STATE_ERR) {
			printf(SUB_1 "PD state error!\n");
			result = false;
			break;
		}
		if (osdp_millis_since(start) > 2 * 1000) {
			printf(SUB_1 "test timout!\n");
			result = false;
			break;
		}
	}
	printf(SUB_1 "CP - PD mixed tests complete\n");

	TEST_REPORT(t, result);

	test_mixed_fsm_teardown(t);
}
