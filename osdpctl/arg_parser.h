#ifndef _ARG_PARSER_H_
#define _ARG_PARSER_H_

#include <stdlib.h>

#define AP_OPT_NOFLAG		0x00000000
#define AP_OPT_REQUIRED		0x00000001
#define AP_OPT_SEEN		0x10000000

enum ap_type_e {
	AP_TYPE_CMD,
	AP_TYPE_BOOL,
	AP_TYPE_INT,
	AP_TYPE_STR,
	AP_TYPE_HEX,
	AP_TYPE_SENTINEL
};

struct ap_option {
	char short_name;
	const char *long_name;
	enum ap_type_e type;
	size_t offset;
	size_t length;
	int flags;
	int (*validator)(void *data);
	int (*handler)(int argc, char **argv);
	const char *help;
};

#define AP_STORE_INT(s, m
#define AP_STORE_STR(s, m, l)	AP_TYPE_STR,  (size_t)&(((s *)0)->m), (l)
#define AP_STORE_BOOL(s, m)	AP_TYPE_BOOL, (size_t)&(((s *)0)->m), (sizeof(int))
#define AP_STORE_HEX(s, m, l)	AP_TYPE_HEX,  (size_t)&(((s *)0)->m), (l)

/* dummy macros to improve readability */
#define AP_ARG(s, l) s, l
#define AP_FLAGS(x) x
#define AP_VALIDATOR(x) x, NULL
#define AP_HELP(x) x
#define AP_CMD(x, y) -1, x, AP_TYPE_CMD, 0, 0, 0, NULL, y
#define AP_SENTINEL  { '\0', NULL, AP_TYPE_SENTINEL, 0, 0, 0, NULL, NULL, NULL }

void ap_init(const char *app_name, const char *app_desc);
int ap_parse(int ac, char **av, struct ap_option *ap_opts, void *data);

#endif
