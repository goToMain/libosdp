/*
 * Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

#define pyosdp_cp_pd_is_online_doc \
	"Get PD status, (online/offline)\n" \
	"\n" \
	"@param pd PD offset number\n" \
	"\n" \
	"@return Secure Channel Status (Bool)"
static PyObject *pyosdp_cp_pd_is_online(pyosdp_t *self, PyObject *args)
{
	int pd;
	uint32_t mask;

	if (!PyArg_ParseTuple(args, "I", &pd))
		return NULL;

	if (pd < 0 || pd > self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		return NULL;
	}

	mask = osdp_get_status_mask(self->ctx);

	if (mask & 1 << pd)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}

#define pyosdp_cp_pd_is_sc_active_doc \
	"Get PD Secure Channel status, (active/inactive)\n" \
	"\n" \
	"@param pd PD offset number\n" \
	"\n" \
	"@return Secure Channel Status (Bool)"
static PyObject *pyosdp_cp_pd_is_sc_active(pyosdp_t *self, PyObject *args)
{
	int pd;
	uint32_t mask;

	if (!PyArg_ParseTuple(args, "I", &pd))
		return NULL;

	if (pd < 0 || pd > self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		return NULL;
	}

	mask = osdp_get_sc_status_mask(self->ctx);

	if (mask & 1 << pd)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}

int pyosdp_cp_event_cb(void *data, int address, struct osdp_event *event)
{
	pyosdp_t *self = data;
	PyObject *arglist, *result, *event_dict;

	if (pyosdp_make_event_dict(&event_dict, event))
		return -1;

	arglist = Py_BuildValue("(IO)", address, event_dict);

	result = PyEval_CallObject(self->event_cb, arglist);

	Py_XDECREF(result);
	Py_DECREF(arglist);
	return 0;
}

#define pyosdp_cp_set_event_callback_doc \
	"Set OSDP event callback handler\n" \
	"\n" \
	"@param callback A function to call when a PD reports an event\n" \
	"\n" \
	"@return None"
static PyObject *pyosdp_cp_set_event_callback(pyosdp_t *self, PyObject *args)
{
	PyObject *event_cb = NULL;

	if (!PyArg_ParseTuple(args, "O", &event_cb))
		return NULL;

	if (!event_cb || !PyCallable_Check(event_cb)) {
		PyErr_SetString(PyExc_TypeError, "Need a callable object!");
		return NULL;
	}

	Py_XDECREF(self->event_cb); /* if set_callback was called earlier */
	self->event_cb = event_cb;
	Py_INCREF(self->event_cb);
	Py_RETURN_NONE;
}

#define pyosdp_cp_refresh_doc \
	"OSDP periodic refresh hook. Must be called at least once every 50ms\n" \
	"\n" \
	"@return None\n"
static PyObject *pyosdp_cp_refresh(pyosdp_t *self, pyosdp_t *args)
{
	osdp_cp_refresh(self->ctx);

	Py_RETURN_NONE;
}

#define pyosdp_cp_send_command_doc \
	"Send an OSDP command to a PD\n" \
	"\n" \
	"@param pd PD offset number\n" \
	"@param command A dict of command keys and values. See osdp.h for details\n" \
	"\n" \
	"@return None\n"
static PyObject *pyosdp_cp_send_command(pyosdp_t *self, PyObject *args)
{
	int pd;
	PyObject *cmd_dict;
	struct osdp_cmd cmd;

	if (!PyArg_ParseTuple(args, "IO!", &pd, &PyDict_Type, &cmd_dict))
		return NULL;

	if (pd < 0 || pd >= self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		return NULL;
	}

	memset(&cmd, 0, sizeof(struct osdp_cmd));
	if (pyosdp_cmd_make_struct(&cmd, cmd_dict))
		return NULL;

	if (osdp_cp_send_command(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_RuntimeError, "send command failed");
		return NULL;
	}

	Py_RETURN_NONE;
}

static int pyosdp_cp_tp_clear(pyosdp_t *self)
{
	Py_XDECREF(self->event_cb);
	return 0;
}

static PyObject *pyosdp_cp_tp_new(PyTypeObject *type, PyObject *args,
				  PyObject *kwargs)
{
	pyosdp_t *self = NULL;

	self = (pyosdp_t *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}
	self->ctx = NULL;
	self->event_cb = NULL;
	self->command_cb = NULL;
	self->num_pd = 0;
	return (PyObject *)self;
}

static void pyosdp_cp_tp_dealloc(pyosdp_t *self)
{
	if (self->ctx)
		osdp_cp_teardown(self->ctx);
	channel_manager_teardown(&self->chn_mgr);
	pyosdp_cp_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

#define pyosdp_cp_tp_init_doc \
	"OSDP Control Panel Class\n" \
	"\n" \
	"@param pd_info List of PD info dicts. See osdp_pd_info_t in osdp.h for more info\n" \
	"@param master_key A hexadecimal string representation of the master key\n" \
	"\n" \
	"@return None"
static int pyosdp_cp_tp_init(pyosdp_t *self, PyObject *args, PyObject *kwargs)
{
	int i, ret=-1, tmp;
	uint8_t *master_key;
	Py_ssize_t master_key_len = 0;
	enum channel_type channel_type;
	PyObject *py_info_list, *py_info;
	static char *kwlist[] = { "", "master_key", NULL };
	char *device = NULL, *channel_type_str = NULL;
	osdp_t *ctx;
	osdp_pd_info_t *info, *info_list = NULL;

	srand(time(NULL));

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|y#:cp_init", kwlist,
					 &PyList_Type, &py_info_list,
					 &master_key, &master_key_len))
		goto error;

	if (master_key_len == 0)
		master_key = NULL;
	if (master_key && master_key_len != 16) {
		PyErr_SetString(PyExc_TypeError, "master_key must be exactly 16 bytes");
		goto error;
	}

	channel_manager_init(&self->chn_mgr);

	self->num_pd = (int)PyList_Size(py_info_list);
	if (self->num_pd == 0 || self->num_pd > 127) {
		PyErr_SetString(PyExc_ValueError, "Invalid num_pd");
		goto error;
	}

	info_list = calloc(self->num_pd, sizeof(osdp_pd_info_t));
	if (info_list == NULL) {
		PyErr_SetString(PyExc_MemoryError, "pd_info alloc error");
		goto error;
	}

	for (i = 0; i < self->num_pd; i++) {
		info = info_list + i;
		py_info = PyList_GetItem(py_info_list, i);
		if (py_info == NULL) {
			PyErr_SetString(PyExc_ValueError,
					"py_info_list extract error");
			goto error;
		}

		if (pyosdp_dict_get_int(py_info, "address", &info->address))
			goto error;

		if (pyosdp_dict_get_int(py_info, "flags", &info->flags))
			goto error;

		if (pyosdp_dict_get_int(py_info, "channel_speed", &info->baud_rate))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_type", &channel_type_str))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_device", &device))
			goto error;

		info->cap = NULL;
		info->scbk = NULL;

		channel_type = channel_guess_type(channel_type_str);
		if (channel_type == CHANNEL_TYPE_ERR) {
			PyErr_SetString(PyExc_ValueError,
					"unable to guess channel type");
			goto error;
		}

		tmp = channel_open(&self->chn_mgr, channel_type, device,
				   info->baud_rate, 0);
		if (tmp != CHANNEL_ERR_NONE &&
		    tmp != CHANNEL_ERR_ALREADY_OPEN) {
			PyErr_SetString(PyExc_PermissionError,
					"Unable to open channel");
			goto error;
		}

		channel_get(&self->chn_mgr, device,
			    &info->channel.id,
			    &info->channel.data,
			    &info->channel.send,
			    &info->channel.recv,
			    &info->channel.flush);
	}

	ctx = osdp_cp_setup(self->num_pd, info_list, master_key);
	if (ctx == NULL) {
		PyErr_SetString(PyExc_Exception, "failed to setup CP");
		goto error;
	}

	osdp_cp_set_event_callback(ctx, pyosdp_cp_event_cb, self);

	ret = 0;
	self->ctx = ctx;
error:
	safe_free(info_list);
	safe_free(channel_type_str);
	safe_free(device);
	return ret;
}

PyObject *pyosdp_cp_tp_repr(PyObject *self)
{
	PyObject *py_string;

	py_string = Py_BuildValue("s", "control panel object");
	Py_INCREF(py_string);

	return py_string;
}

static int pyosdp_cp_tp_traverse(pyosdp_t *self, visitproc visit, void *arg)
{
	Py_VISIT(self->ctx);
	return 0;
}

PyObject *pyosdp_cp_tp_str(PyObject *self)
{
	return pyosdp_cp_tp_repr(self);
}

static PyGetSetDef pyosdp_cp_tp_getset[] = {
	{NULL} /* Sentinel */
};

static PyMethodDef pyosdp_cp_tp_methods[] = {
	{
		"refresh",
		(PyCFunction)pyosdp_cp_refresh,
		METH_NOARGS,
		pyosdp_cp_refresh_doc
	},
	{
		"set_event_callback",
		(PyCFunction)pyosdp_cp_set_event_callback,
		METH_VARARGS,
		pyosdp_cp_set_event_callback_doc
	},
	{
		"send_command",
		(PyCFunction)pyosdp_cp_send_command,
		METH_VARARGS,
		pyosdp_cp_send_command_doc
	},
	{
		"is_online",
		(PyCFunction)pyosdp_cp_pd_is_online,
		METH_VARARGS,
		pyosdp_cp_pd_is_online_doc
	},
	{
		"sc_active",
		(PyCFunction)pyosdp_cp_pd_is_sc_active,
		METH_VARARGS,
		pyosdp_cp_pd_is_sc_active_doc
	},
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMemberDef pyosdp_cp_tp_members[] = {
	{ NULL } /* Sentinel */
};

static PyTypeObject ControlPanelTypeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	.tp_name      = "ControlPanel",
	.tp_doc       = pyosdp_cp_tp_init_doc,
	.tp_basicsize = sizeof(pyosdp_t),
	.tp_itemsize  = 0,
	.tp_dealloc   = (destructor)pyosdp_cp_tp_dealloc,
	.tp_repr      = pyosdp_cp_tp_repr,
	.tp_str       = pyosdp_cp_tp_str,
	.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_traverse  = (traverseproc)pyosdp_cp_tp_traverse,
	.tp_clear     = (inquiry)pyosdp_cp_tp_clear,
	.tp_methods   = pyosdp_cp_tp_methods,
	.tp_members   = pyosdp_cp_tp_members,
	.tp_getset    = pyosdp_cp_tp_getset,
	.tp_init      = (initproc)pyosdp_cp_tp_init,
	.tp_new       = pyosdp_cp_tp_new,
};

int pyosdp_add_type_cp(PyObject *module)
{
	return pyosdp_module_add_type(module, "ControlPanel",
				      &ControlPanelTypeObject);
}
