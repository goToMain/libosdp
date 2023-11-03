/*
 * Copyright (c) 2019-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "arg_parser.h"

#define is_lower_alpha(x) (x >= 97 && x <= 122)

#define xstr(s) #s
#define str(s)	xstr(s)
#define PAD	str(AP_HELP_SPACING)

const char *ap_app_name;
const char *ap_app_desc;

void ap_print_help(struct ap_option *ap_opts, int exit_code)
{
	int count;
	char opt_str[64];
	struct ap_option *ap_opt;

	if (ap_app_name && exit_code == 0)
		printf("%s - %s\n", ap_app_name, ap_app_desc);

	printf("\nUsage: %s [OPTIONS...] <CONFIG> <COMMAND> [CMD_ARGS[0] ...]\n"
	       "\nOPTIONS:\n",
	       ap_app_name);

	ap_opt = ap_opts;
	while (ap_opt->short_name != '\0') {
		if (ap_opt->short_name == -1) {
			ap_opt++;
			continue;
		}

		if (ap_opt->type == AP_TYPE_BOOL ||
		    ap_opt->type == AP_TYPE_BOOL_HANDLER) {
			snprintf(opt_str, 64, "%s", ap_opt->long_name);
		} else {
			snprintf(opt_str, 64, "%s <%s>", ap_opt->long_name,
				 ap_opt->opt_name);
		}
		printf("  -%c, --%-" PAD "s %s\n", ap_opt->short_name, opt_str,
		       ap_opt->help);
		ap_opt++;
	}
	printf("  -%c, --%-" PAD "s Fork to background\n", 'f', "fork");
	printf("  -%c, --%-" PAD "s Prevent writing to tty\n", 'q', "quiet");
	printf("  -%c, --%-" PAD "s Print this help message\n", 'h', "help");

	count = 0;
	ap_opt = ap_opts;
	while (ap_opt->short_name != '\0') {
		if (ap_opt->short_name != -1) {
			ap_opt++;
			continue;
		}
		if (count == 0)
			printf("\nCOMMANDS:\n");
		printf("  %-" PAD "s       %s\n", ap_opt->long_name,
		       ap_opt->help);
		ap_opt++;
		count++;
	}

	exit(exit_code);
}

void ap_init(const char *app_name, const char *app_desc)
{
	ap_app_name = app_name;
	ap_app_desc = app_desc;
}

int ap_parse(int argc, char *argv[], struct ap_option *ap_opts, void *data)
{
	char **cval;
	char ostr[128];
	struct option *opts, *opt;
	struct ap_option *ap_opt;
	int i, c, ret, *ival, pid;
	int opts_len = 0, cmd_len = 0, opt_idx = 0, olen = 0, do_fork = 0;
	int ap_opts_len = 0;

	ap_opt = ap_opts;
	while (ap_opt->short_name != '\0') {
		if (ap_opt->short_name != -1)
			opts_len++;
		else
			cmd_len++;
		ap_opts_len++;
		ap_opt++;
	}

	opts = malloc(sizeof(struct option) * (opts_len + 3));
	if (opts == NULL) {
		printf("Error: alloc error\n");
		exit(-1);
	}

	i = 0;
	ap_opt = ap_opts;
	while (ap_opt->short_name != '\0') {
		if (ap_opt->short_name == -1) {
			ap_opt++;
			continue;
		}
		opt = opts + i++;
		opt->name = ap_opt->long_name;
		if (ap_opt->type == AP_TYPE_BOOL ||
		    ap_opt->type == AP_TYPE_BOOL_HANDLER) {
			opt->has_arg = no_argument;
			assert(olen + 2 < 128);
			ostr[olen++] = ap_opt->short_name;
		} else {
			opt->has_arg = required_argument;
			assert(olen + 3 < 128);
			ostr[olen++] = ap_opt->short_name;
			ostr[olen++] = ':';
		}
		opt->flag = 0;
		opt->val = ap_opt->short_name;
		ap_opt++;
	}

	assert(olen + 4 < 128);

	/* add help and other default options */
	opt = opts + i + 0;
	opt->name = "help";
	opt->val = 'h';
	ostr[olen++] = 'h';

	opt = opts + i + 1;
	opt->name = "quiet";
	opt->val = 'q';
	ostr[olen++] = 'q';

	opt = opts + i + 2;
	opt->val = 'f';
	ostr[olen++] = 'f';

	ostr[olen] = '\0';

	while ((c = getopt_long(argc, argv, ostr, opts, &opt_idx)) >= 0) {
		/* find c in ap_opt->short_name */
		for (i = 0; i < ap_opts_len && c != ap_opts[i].short_name; i++)
			;

		if (c == 'h')
			ap_print_help(ap_opts, 0);
		if (c == 'q') {
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			continue;
		}
		if (c == 'f') {
			do_fork = 1;
			continue;
		}
		if (c == '?')
			ap_print_help(ap_opts, -1);

		switch (ap_opts[i].type) {
		case AP_TYPE_BOOL:
			ival = (int *)((char *)data + ap_opts[i].offset);
			*ival = 1;
			break;
		case AP_TYPE_STR:
			cval = (char **)(((char *)data) + ap_opts[i].offset);
			*cval = strdup(optarg);
			break;
		case AP_TYPE_INT:
			ival = (int *)((char *)data + ap_opts[i].offset);
			*ival = atoi(optarg);
			break;
		case AP_TYPE_BOOL_HANDLER:
			ap_opts[i].bool_handler();
			exit(0);
		default:
			printf("Error: invalid arg type %d\n", ap_opts[i].type);
			exit(-1);
		}
		if (ap_opts[i].validator) {
			ret = ap_opts[i].validator(data);
			if (ret != 0)
				exit(-1);
		}
		ap_opts[i].flags |= AP_OPT_SEEN;
	}
	free(opts);

	ap_opt = ap_opts;
	while (ap_opt->short_name != '\0') {
		if (ap_opt->short_name != -1) {
			if (ap_opt->flags & AP_OPT_REQUIRED &&
			    (ap_opt->flags & AP_OPT_SEEN) == 0U) {
				printf("Error: arg '%c' is mandatory\n\n",
				       ap_opt->short_name);
				ap_print_help(ap_opts, -1);
			}
		} else if (argv[optind]) {
			if (strcmp(ap_opt->long_name, argv[optind]) == 0) {
				if (do_fork) {
					if ((pid = fork()) < 0)
						return -1;
					if (pid != 0)
						exit(0);
				}
				return ap_opt->handler(argc - optind - 1,
						       argv + optind + 1, data);
			}
		}
		ap_opt++;
	}

	if (cmd_len) {
		printf("Error: unknown command '%s'\n\n", argv[optind]);
		ap_print_help(ap_opts, -1);
	}

	return optind;
}
