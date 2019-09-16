/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Mon Sep 16 21:59:28 IST 2019
 */

#ifndef _OSDP_TEST_H_
#define _OSDP_TEST_H_

#include <stdio.h>
#include <string.h>
#include "common.h"

#define DO_TEST(t, m) do {          \
        t->tests++;                 \
        if(m(t->mock_data)) {       \
            t->failure++;           \
        } else {                    \
            t->success++;           \
        }                           \
    } while(0)

struct test {
    int success;
    int failure;
    int tests;
    void *mock_data;
};

void run_cp_phy_tests(struct test *t);

#endif
