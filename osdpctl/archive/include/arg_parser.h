/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARG_PARSER_H_
#define _ARG_PARSER_H_

#include <stdlib.h>

#define AP_HELP_SPACING 25

#define AP_OPT_NOFLAG	0x00000000
#define AP_OPT_REQUIRED 0x00000001
#define AP_OPT_SEEN	0x10000000

enum ap_type_e {
	AP_TYPE_CMD,
	AP_TYPE_BOOL,
	AP_TYPE_INT,
	AP_TYPE_STR,
	AP_TYPE_BOOL_HANDLER,
	AP_TYPE_SENTINEL
};

struct ap_option {
	signed char short_name;
	const char *long_name;
	const char *opt_name;
	enum ap_type_e type;
	size_t offset;

	int flags;
	int (*validator)(void *data);
	int (*handler)(int argc, char *argv[], void *data);
	const char *help;
	void (*bool_handler)();
};

#define AP_STORE_INT(s, m)  AP_TYPE_INT, (size_t) & (((s *)0)->m)
#define AP_STORE_STR(s, m)  AP_TYPE_STR, (size_t) & (((s *)0)->m)
#define AP_STORE_BOOL(s, m) AP_TYPE_BOOL, (size_t) & (((s *)0)->m)

#define AP_CMD(x, y) -1, x, NULL, AP_TYPE_CMD, 0, 0, NULL, y
#define AP_ARG_BOOL(s, l, h, m)                                                \
	s, l, NULL, AP_TYPE_BOOL_HANDLER, 0, 0, NULL, NULL, m, h
#define AP_SENTINEL                                                            \
	{                                                                      \
		'\0', NULL, NULL, AP_TYPE_SENTINEL, 0, 0, NULL, NULL, NULL,    \
			NULL                                                   \
	}

/* dummy macros to improve readability */
#define AP_ARG(s, l, n) s, l, n
#define AP_VALIDATOR(x) x, NULL
#define AP_FLAGS(x)	x
#define AP_HELP(x)	x, NULL

void ap_init(const char *app_name, const char *app_desc);
int ap_parse(int argc, char *argv[], struct ap_option *ap_opts, void *data);
void ap_print_help(struct ap_option *ap_opts, int exit_code);

#endif
