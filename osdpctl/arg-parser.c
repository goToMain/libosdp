#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "arg-parser.h"

#define is_lower_alpha(x) (x >= 97 && x <= 122)

const char *ap_app_name;
const char *ap_app_desc;

void ap_init(const char *app_name, const char *app_desc)
{
	ap_app_name = app_name;
	ap_app_desc = app_desc;
}

void ap_print_help(struct ap_option *ap_opts)
{
	int i, l;
	char tmp1[64], tmp2[64];
	struct ap_option *ap_opt = ap_opts;

	if (ap_app_name)
		printf("%s - %s\n\n", ap_app_name, ap_app_desc);

	printf("Arguments:\n");
	while (ap_opt != NULL) {
		if (ap_opt->short_name == '\0')
			break;
		if (ap_opt->flags & AP_TYPE_BOOL) {
			printf("  -%c, --%-28s \t%s\n",
			       ap_opt->short_name, ap_opt->long_name,
			       ap_opt->help);
		} else {
			strncpy(tmp1, ap_opt->long_name, 63);
			tmp1[63] = 0;
			l = strlen(tmp1);
			for (i = 0; i < l; i++) {
				if (is_lower_alpha(tmp1[i]))
					tmp1[i] &= ~0x20;
			}
			snprintf(tmp2, 64, "--%s <%s>", ap_opt->long_name, tmp1);
			printf("  -%c, %-30s \t%s\n", ap_opt->short_name, tmp2,
			       ap_opt->help);
		}
		ap_opt++;
	}
	printf("  -h, --%-28s \tPrint this help message\n", "help");
}

int hex_string_to_array(const char *hex_string, unsigned char *arr, int maxlen)
{
	int i, len;
	unsigned char tmp[64];

	len = strlen(hex_string) / 2;
	if (len > 64)
		return -1;

	if (len > maxlen)
	len = maxlen;

	for (i=0; i<len; i++) {
		if (sscanf(&(hex_string[i * 2]), "%2hhx", &(tmp[i])) != 1)
			return -1;
	}

	memcpy(arr, tmp, len);
	return 0;
}


int ap_parse(int ac, char **av, struct ap_option *ap_opts, void *data)
{
	char *cval;
	char ostr[128];
	struct option *opts, *opt;
	struct ap_option *ap_opt;
	int i, c, r, len, opt_idx=0, olen=0, *ival;

	len = 0;
	while (ap_opts[len++].short_name != '\0');
	if (len == 0) {
		printf("Invalid ap_opts\n");
		exit(-1);
	}

	opts = malloc(sizeof (struct option) * len + 1);
	if (opts == NULL) {
		printf("AP Alloc error\n");
		exit(-1);
	}

	for (i = 0; i < len; i++) {
		ap_opt = ap_opts + i;
		opt = opts + i;
		opt->name = ap_opts->long_name;
		if (ap_opt->flags & AP_TYPE_BOOL) {
			opt->has_arg = no_argument;
			olen += sprintf(ostr + olen, "%c", ap_opt->short_name);
		} else {
			opt->has_arg = required_argument;
			olen += sprintf(ostr + olen, "%c:", ap_opt->short_name);
		}
		opt->flag = 0;
		opt->val = ap_opt->short_name;
	}

	opt = opts + i;
	opt->name = "help";
	opt->val = 'h';
	sprintf(ostr + olen, "h");

	while ((c = getopt_long(ac, av, ostr, opts, &opt_idx)) >= 0) {
		for (i = 0; i < len; i++) {
			if (c == ap_opts[i].short_name)
				break;
		}
		if (c == 'h') {
			ap_print_help(ap_opts);
			exit(0);
		}
		if (i >= len) {
			ap_print_help(ap_opts);
			exit(-1);
		}

		switch (ap_opts[i].type) {
		case AP_TYPE_BOOL:
			ival = (int *)((char *)data + ap_opts[i].offset);
			*ival = 1;
			break;
		case AP_TYPE_STR:
			cval = (char *)((char *)data + ap_opts[i].offset);
			strncpy(cval, optarg, ap_opts[i].length - 1);
			cval[ap_opts[i].length - 1] = 0;
			break;
		case AP_TYPE_INT:
			ival = (int *)((char *)data + ap_opts[i].offset);
			*ival = atoi(optarg);
			break;
		case AP_TYPE_HEX:
			cval = (char *)((char *)data + ap_opts[i].offset);
			r = hex_string_to_array(optarg, (unsigned char *)cval,
						ap_opts[i].length);
			if (r != 0) {
				printf("Error parsing argument '%c'\n", c);
				exit(-1);
			}
			break;
		default:
			printf("AP invalid type\n");
			exit(-1);
		}
		if (ap_opts[i].validator) {
			r = ap_opts[i].validator(data);
			if (r != 0)
				exit(-1);
		}
		ap_opts[i].flags |= AP_OPT_SEEN;
	}

	for (i = 0; i < len; i++) {
		if (ap_opts[i].flags & AP_OPT_REQUIRED &&
		    (ap_opts[i].flags & AP_OPT_SEEN) == 0U) {
			printf("Mandatory arg '%c' not provided\n",
				ap_opts[i].short_name);
			ap_print_help(ap_opts);
			exit(-1);
		}
	}

	free(opts);
	return 0;
}
