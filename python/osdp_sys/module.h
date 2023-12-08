/*
 * Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PYOSDP_H_
#define _PYOSDP_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#include <utils/utils.h>
#include <utils/strutils.h>
#include <utils/hashmap.h>
#include <utils/channel.h>
#include <utils/memory.h>
#include <osdp.h>

typedef struct {
	PyObject_HEAD
	struct channel_manager channel_manager;
	bool is_cp;

	int file_id;
	struct {
		PyObject *open_cb;
		PyObject *read_cb;
		PyObject *write_cb;
		PyObject *close_cb;
	} fops;
} pyosdp_base_t;

typedef struct {
	pyosdp_base_t base;
	PyObject *event_cb;
	int num_pd;
	osdp_t *ctx;
	char *name;
} pyosdp_cp_t;

typedef struct {
	pyosdp_base_t base;
	PyObject *command_cb;
	osdp_t *ctx;
	char *name;
} pyosdp_pd_t;

#define DBG_PRINT_Py_REFCNT(x) \
	fprintf(stderr, "%s: %s:%d Py_REFCNT(%s): %ld (%p)\n", TAG, __FUNCTION__, __LINE__, STR(X) , Py_REFCNT(x), x);

/* from pyosdp_utils.c */

int pyosdp_module_add_type(PyObject *module, const char *name,
			   PyTypeObject *type);

int pyosdp_parse_int(PyObject *obj, int *res);
int pyosdp_parse_str(PyObject *obj, char **str);
int pyosdp_parse_bytes(PyObject *obj, uint8_t **data, int *length);

int pyosdp_dict_get_bool(PyObject *dict, const char *key, bool *res);
int pyosdp_dict_get_int(PyObject *dict, const char *key, int *res);
int pyosdp_dict_get_str(PyObject *dict, const char *key, char **str);
int pyosdp_dict_get_bytes(PyObject *dict, const char *key, uint8_t **buf,
			  int *len);
int pyosdp_dict_get_object(PyObject *dict, const char *key, PyObject **obj);

int pyosdp_dict_add_bool(PyObject *dict, const char *key, bool val);
int pyosdp_dict_add_int(PyObject *dict, const char *key, int val);
int pyosdp_dict_add_str(PyObject *dict, const char *key, const char *val);
int pyosdp_dict_add_bytes(PyObject *dict, const char *key, const uint8_t *data,
			  int len);

/* from pyosdp_base.c */

extern PyTypeObject OSDPBaseType;
int pyosdp_add_type_osdp_base(PyObject *module);

/* from pyosdp_cp.c */

int pyosdp_add_type_cp(PyObject *module);

/* from pyosdp_pd.c */

int pyosdp_add_type_pd(PyObject *module);

/* from pyosdp_cmd.c */

int pyosdp_make_struct_cmd(struct osdp_cmd *cmd, PyObject *dict);
int pyosdp_make_dict_cmd(PyObject **dict, struct osdp_cmd *cmd);
int pyosdp_make_dict_event(PyObject **dict, struct osdp_event *event);
int pyosdp_make_struct_event(struct osdp_event *event, PyObject *dict);

#endif /* _PYOSDP_H_ */
