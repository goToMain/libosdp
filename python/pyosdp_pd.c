/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

static const char pyosdp_pd_tp_doc[] =
"\n"
;

static PyObject *pyosdp_pd_get_command(pyosdp_t *self, PyObject *args)
{
	Py_RETURN_NONE;
}

static PyObject *pyosdp_pd_set_loglevel(pyosdp_t *self, PyObject *args)
{
	int log_level;

	if (!PyArg_ParseTuple(args, "I", &log_level))
		return NULL;

	if (log_level <= 0 || log_level > 7) {
		PyErr_SetString(PyExc_KeyError, "invalid log level");
		return NULL;
	}

	osdp_set_log_level(log_level);

	Py_RETURN_NONE;
}

static PyObject *pyosdp_pd_refresh(pyosdp_t *self, PyObject *args)
{
	osdp_pd_refresh(self->ctx);

	Py_RETURN_NONE;
}

static int pyosdp_pd_tp_clear(pyosdp_t *self)
{
	return 0;
}

static PyObject *pyosdp_pd_tp_new(PyTypeObject *type, PyObject *args,
			          PyObject *kwargs)
{
	pyosdp_t *self = NULL;

	self = (pyosdp_t *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}
	self->ctx = NULL;
	return (PyObject *)self;
}

static void pyosdp_pd_tp_dealloc(pyosdp_t *self)
{
	osdp_pd_teardown(self->ctx);
	channel_manager_teardown(&self->chn_mgr);
	pyosdp_pd_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static int pyosdp_add_pd_cap(PyObject *obj, osdp_pd_info_t *info)
{
	PyObject *py_pd_cap;
	int i, cap_list_size, function_code, compliance_level, num_items;

	cap_list_size = (int)PyList_Size(obj);
	if (cap_list_size == 0 || cap_list_size >= OSDP_PD_CAP_SENTINEL) {
		PyErr_SetString(PyExc_ValueError, "Invalid cap list size");
		return -1;
	}

	info->cap = calloc(cap_list_size + 1, sizeof(struct osdp_pd_cap));
	if (info->cap == NULL) {
		PyErr_SetString(PyExc_MemoryError, "pd cap alloc error");
		return -1;
	}

	for (i = 0; i < cap_list_size; i++) {
		py_pd_cap = PyList_GetItem(obj, i);

		if (pyosdp_dict_get_int(py_pd_cap, "function_code",
					&function_code))
			return -1;

		if (pyosdp_dict_get_int(py_pd_cap, "compliance_level",
					&compliance_level))
			return -1;

		if (pyosdp_dict_get_int(py_pd_cap, "num_items",
					&num_items))
			return -1;

		info->cap[i].function_code = (uint8_t)function_code;
		info->cap[i].compliance_level = (uint8_t)compliance_level;
		info->cap[i].num_items = (uint8_t)num_items;
	}

	return 0;
}

static int pyosdp_pd_tp_init(pyosdp_t *self, PyObject *args, PyObject *kwargs)
{
	int ret;
	osdp_t *ctx;
	osdp_pd_info_t info;
	enum channel_type channel_type;
	char *device = NULL, *channel_type_str = NULL, *scbk_str = NULL;
	uint8_t scbk[16] = {0};
	static char *kwlist[] = { "", "capabilities", "scbk", NULL };
	PyObject *py_info, *py_pd_cap_list;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|$O!s", kwlist,
					 &PyDict_Type, &py_info,
					 &PyList_Type, &py_pd_cap_list,
					 &scbk_str))
		goto error;

	if (scbk_str && (strlen(scbk_str) != 32 ||
	    hstrtoa(scbk, scbk_str) == -1)) {
		PyErr_SetString(PyExc_TypeError, "master_key parse error");
		goto error;
	}

	if (py_pd_cap_list && pyosdp_add_pd_cap(py_pd_cap_list, &info))
		goto error;

	channel_manager_init(&self->chn_mgr);

	if (pyosdp_dict_get_int(py_info, "address", &info.address))
		goto error;

	if (pyosdp_dict_get_int(py_info, "channel_speed", &info.baud_rate))
		goto error;

	if (pyosdp_dict_get_str(py_info, "channel_type", &channel_type_str))
		goto error;

	if (pyosdp_dict_get_str(py_info, "channel_device", &device))
		goto error;

	info.flags = 0;
	info.cap = NULL;

	channel_type = channel_guess_type(channel_type_str);
	if (channel_type == CHANNEL_TYPE_ERR) {
		PyErr_SetString(PyExc_ValueError, "unable to guess channel type");
		goto error;
	}

	ret = channel_open(&self->chn_mgr, channel_type, device, info.baud_rate, 1);
	if (ret != CHANNEL_ERR_NONE &&
	    ret != CHANNEL_ERR_ALREADY_OPEN) {
		PyErr_SetString(PyExc_PermissionError, "Unable to open channel");
		goto error;
	}

	channel_get(&self->chn_mgr, device,
		&info.channel.data,
		&info.channel.send,
		&info.channel.recv,
		&info.channel.flush);

	ctx = osdp_pd_setup(&info, scbk_str ? scbk : NULL);
	if (ctx == NULL) {
		PyErr_SetString(PyExc_Exception, "failed to setup pd");
		goto error;
	}

	self->ctx = ctx;
	safe_free(channel_type_str);
	safe_free(device);
	return 0;
error:
	safe_free(channel_type_str);
	safe_free(device);
	return -1;
}

PyObject *pyosdp_pd_tp_repr(PyObject *self)
{
	PyObject *py_string;

	py_string = Py_BuildValue("s", "control panel object");
	Py_INCREF(py_string);

	return py_string;
}

static int pyosdp_pd_tp_traverse(pyosdp_t *self, visitproc visit, void *arg)
{
	Py_VISIT(self->ctx);
	return 0;
}

PyObject *pyosdp_pd_tp_str(PyObject *self)
{
	return pyosdp_pd_tp_repr(self);
}

static PyGetSetDef pyosdp_pd_tp_getset[] = {
	{NULL} /* Sentinel */
};

static PyMethodDef pyosdp_pd_tp_methods[] = {
	{
		"refresh",
		(PyCFunction)pyosdp_pd_refresh,
		METH_NOARGS,
		"Periodic refresh hook"
	},
	{
		"set_loglevel",
		(PyCFunction)pyosdp_pd_set_loglevel,
		METH_VARARGS,
	  	"Set osdp event callbacks"
	},
	{
		"get_command",
		(PyCFunction)pyosdp_pd_get_command,
		METH_VARARGS,
	  	"Set osdp event callbacks"
	},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

static PyMemberDef pyosdp_pd_tp_members[] = {
	{NULL} /* Sentinel */
};

PyTypeObject PeripheralDeviceTypeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"PeripheralDevice",				/* tp_name */
	sizeof(pyosdp_t),				/* tp_basicsize */
	0,						/* tp_itemsize */
	(destructor)pyosdp_pd_tp_dealloc,		/* tp_dealloc */
	0,						/* tp_print */
	0,						/* tp_getattr */
	0,						/* tp_setattr */
	0,						/* tp_compare */
	pyosdp_pd_tp_repr,				/* tp_repr */
	0,						/* tp_as_number */
	0,						/* tp_as_sequence */
	0,						/* tp_as_mapping */
	0,						/* tp_hash */
	0,						/* tp_call */
	pyosdp_pd_tp_str,				/* tp_str */
	0,						/* tp_getattro */
	0,						/* tp_setattro */
	0,						/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
	pyosdp_pd_tp_doc,				/* tp_doc */
	(traverseproc)pyosdp_pd_tp_traverse,		/* tp_traverse */
	(inquiry)pyosdp_pd_tp_clear,			/* tp_clear */
	0,						/* tp_richcompare */
	0,						/* tp_weaklistoffset */
	0,						/* tp_iter */
	0,						/* tp_iternext */
	pyosdp_pd_tp_methods,				/* tp_methods */
	pyosdp_pd_tp_members,				/* tp_members */
	pyosdp_pd_tp_getset,				/* tp_getset */
	0,						/* tp_base */
	0,						/* tp_dict */
	0,						/* tp_descr_get */
	0,						/* tp_descr_set */
	0,						/* tp_dictoffset */
	(initproc)pyosdp_pd_tp_init,			/* tp_init */
	0,						/* tp_alloc */
	pyosdp_pd_tp_new,				/* tp_new */
	0,						/* tp_free */
	0						/* tp_is_gc */
};

int pyosdp_add_type_pd(PyObject *module)
{
	return pyosdp_module_add_type(module, "PeripheralDevice",
				      &PeripheralDeviceTypeObject);
}
