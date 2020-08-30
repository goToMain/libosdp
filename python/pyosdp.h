/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PYOSDP_H_
#define _PYOSDP_H_

#include <Python.h>
#include <structmember.h>
#include <utils/hashmap.h>
#include <utils/channel.h>
#include <osdp.h>


typedef struct {
	PyObject_HEAD
	PyObject *keypress_cb;
	PyObject *cardread_cb;
	osdp_t *ctx;
	struct channel_manager chn_mgr;
	int num_pd;
} pyosdp_t;

/* from common.c */

int pyosdp_module_add_type(PyObject *module, const char *name,
			   PyTypeObject *type);
int pyosdp_build_pd_info(osdp_pd_info_t *info, int address, int baud_rate);

/* from pyosdp_cp.c */

int pyosdp_add_type_cp(PyObject *module);

#endif /* _PYOSDP_H_ */
