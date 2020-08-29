/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PYOSDP_COMMON_H_
#define _PYOSDP_COMMON_H_

#include <Python.h>
#include <structmember.h>

#include <osdp.h>

typedef struct {
	PyObject_HEAD
	osdp_pd_info_t *info;
	PyObject *keypress_cb;
	PyObject *cardread_cb;
	osdp_t *ctx;
	int num_pd;
} pyosdp_t;

/* from common.c */
int pyosdp_module_add_type(PyObject *module, const char *name,
			   PyTypeObject *type);
int pyosdp_build_pd_info(osdp_pd_info_t *info, int address, int baud_rate);
void pyosdp_close_channel(struct osdp_channel *channel);
int pyosdp_open_channel(struct osdp_channel *channel, const char *device,
			int baud_rate);

/* from pyosdp_cp.c */
int pyosdp_add_type_cp(PyObject *module);

#endif /* _PYOSDP_COMMON_H_ */
