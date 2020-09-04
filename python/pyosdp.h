/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PYOSDP_H_
#define _PYOSDP_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <structmember.h>
#include <utils/utils.h>
#include <utils/strutils.h>
#include <utils/hashmap.h>
#include <utils/channel.h>
#include <osdp.h>

typedef struct {
	PyObject_HEAD
	PyObject *command_cb;
	PyObject *keypress_cb;
	PyObject *cardread_cb;
	osdp_t *ctx;
	struct channel_manager chn_mgr;
	int num_pd;
} pyosdp_t;

/* from pyosdp_utils.c */

int pyosdp_module_add_type(PyObject *module, const char *name, PyTypeObject *type);

int pyosdp_parse_int(PyObject *obj, int *res);
int pyosdp_parse_str(PyObject *obj, char **str);

int pyosdp_dict_get_int(PyObject *dict, const char *key, int *res);
int pyosdp_dict_get_str(PyObject *dict, const char *key, char **str);

int pyosdp_dict_add_int(PyObject *dict, const char *key, int val);
int pyosdp_dict_add_str(PyObject *dict, const char *key, const char *val);

/* from pyosdp_cp.c */

int pyosdp_add_type_cp(PyObject *module);

/* from pyosdp_pd.c */

int pyosdp_add_type_pd(PyObject *module);

/* from pyosdp_cmd.c */

int pyosdp_cmd_make_struct(struct osdp_cmd *cmd, PyObject *dict);
int pyosdp_cmd_make_dict(PyObject **dict, struct osdp_cmd *cmd);

#endif /* _PYOSDP_H_ */
