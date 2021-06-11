/*
 * Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

#define pyosdp_pd_is_sc_active_doc \
	"Get Secure Channel status, (active/inactive)\n" \
	"\n" \
	"@return Secure Channel Status (Bool)"
static PyObject *pyosdp_pd_is_sc_active(pyosdp_t *self, PyObject *args)
{
	uint32_t mask;

	mask = osdp_get_sc_status_mask(self->ctx);

	if (mask & 1)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}

#define pyosdp_pd_notify_event_doc \
	"Notify the CP of an OSDP event\n" \
	"\n" \
	"@param event A dict of event keys and values. See osdp.h for details\n" \
	"\n" \
	"@return None\n"
static PyObject *pyosdp_pd_notify_event(pyosdp_t *self, PyObject *args)
{
	PyObject *event_dict;
	struct osdp_event event;

	if (!PyArg_ParseTuple(args, "O", &event_dict))
		return NULL;

	if (pyosdp_make_event_struct(&event, event_dict))
		return NULL;

	if (osdp_pd_notify_event(self->ctx, &event)) {
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

static int pd_command_cb(void *arg, struct osdp_cmd *cmd)
{
	int ret_val = -1;
	pyosdp_t *self = arg;
	PyObject *dict, *arglist, *result;

	if (pyosdp_cmd_make_dict(&dict, cmd))
		return -1;

	arglist = Py_BuildValue("(O)", dict);
	result = PyEval_CallObject(self->command_cb, arglist);

	if (result && PyDict_Check(result)) {
		if (pyosdp_dict_get_int(result, "return_code", &ret_val) == 0) {
			/**
			 * If ret_val > 0 and we can make a MFGREP command
			 * out of the dict from python, we will reply with it.
			 * If not, reply with NAK.
			 */
			if (ret_val > 0) {
				memset(cmd, 0, sizeof(struct osdp_cmd));
				if (pyosdp_cmd_make_struct(cmd, result) < 0)
					ret_val = -1;
				else if (cmd->id != OSDP_CMD_MFG)
					ret_val = -1;
			}
		}
	}

	Py_XDECREF(dict);
	Py_XDECREF(result);
	Py_DECREF(arglist);
	return ret_val;
}

#define pyosdp_pd_set_command_callback_doc \
	"Set OSDP command callback handler\n" \
	"\n" \
	"@param callback A function to call when a CP sends a command\n" \
	"\n" \
	"@return None"
static PyObject *pyosdp_pd_set_command_callback(pyosdp_t *self, PyObject *args)
{
	PyObject *callable = NULL;

	if (!PyArg_ParseTuple(args, "O", &callable))
		return NULL;

	if (callable == NULL || !PyCallable_Check(callable)) {
		PyErr_SetString(PyExc_TypeError, "Need a callable object!");
		return NULL;
	}

	self->command_cb = callable;
	Py_INCREF(self->command_cb);
	osdp_pd_set_command_callback(self->ctx, pd_command_cb, (void *)self);
	Py_RETURN_NONE;
}

#define pyosdp_pd_refresh_doc \
	"OSDP periodic refresh hook. Must be called at least once every 50ms\n" \
	"\n" \
	"@return None\n"
static PyObject *pyosdp_pd_refresh(pyosdp_t *self, PyObject *args)
{
	osdp_pd_refresh(self->ctx);

	Py_RETURN_NONE;
}

static int pyosdp_pd_tp_clear(pyosdp_t *self)
{
	Py_XDECREF(self->command_cb);
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
	self->event_cb = NULL;
	self->command_cb = NULL;
	self->num_pd = 0;
	return (PyObject *)self;
}

static void pyosdp_pd_tp_dealloc(pyosdp_t *self)
{
	if (self->ctx)
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
	if (cap_list_size == 0)
		return 0;
	if (cap_list_size >= OSDP_PD_CAP_SENTINEL) {
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

#define pyosdp_pd_tp_init_doc \
	"OSDP Peripheral Device Class\n" \
	"\n" \
	"@param pd_info A dict with osdp_pd_info_t keys and valuse. See osdp.h for more info.\n" \
	"@param capabilities A list of osdp_pd_cap_t keys and valuse. See osdp.h for more details.\n" \
	"@param scbk A hexadecimal string representation of the PD secure channel base key\n" \
	"\n" \
	"@return None"
static int pyosdp_pd_tp_init(pyosdp_t *self, PyObject *args, PyObject *kwargs)
{
	int ret, scbk_length;
	osdp_t *ctx;
	osdp_pd_info_t info;
	enum channel_type channel_type;
	char *device = NULL, *channel_type_str = NULL;
	static char *kwlist[] = { "", "capabilities", "scbk", NULL };
	PyObject *py_info, *py_pd_cap_list;
	uint8_t *scbk = NULL;

	srand(time(NULL));

	info.cap = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|$O!", kwlist,
					 &PyDict_Type, &py_info,
					 &PyList_Type, &py_pd_cap_list))
		goto error;

	if (py_pd_cap_list && pyosdp_add_pd_cap(py_pd_cap_list, &info))
		goto error;

	channel_manager_init(&self->chn_mgr);

	if (pyosdp_dict_get_int(py_info, "address", &info.address))
		goto error;

	if (pyosdp_dict_get_int(py_info, "flags", &info.flags))
		goto error;

	if (pyosdp_dict_get_int(py_info, "channel_speed", &info.baud_rate))
		goto error;

	if (pyosdp_dict_get_str(py_info, "channel_type", &channel_type_str))
		goto error;

	if (pyosdp_dict_get_str(py_info, "channel_device", &device))
		goto error;

	if (pyosdp_dict_get_int(py_info, "version", &info.id.version))
		goto error;

	if (pyosdp_dict_get_int(py_info, "model", &info.id.model))
		goto error;

	if (pyosdp_dict_get_int(py_info, "vendor_code", (int *)&info.id.vendor_code))
		goto error;

	if (pyosdp_dict_get_int(py_info, "firmware_version", (int *)&info.id.firmware_version))
		goto error;

	if (pyosdp_dict_get_int(py_info, "serial_number", (int *)&info.id.serial_number))
		goto error;

	if (pyosdp_dict_get_bytes(py_info, "scbk", &scbk, &scbk_length))
		goto error;

	info.scbk = NULL;
	if (scbk && scbk_length == 16)
		info.scbk = scbk;

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

	channel_get(&self->chn_mgr, device, &info.channel.id, &info.channel.data,
		    &info.channel.send, &info.channel.recv, &info.channel.flush);

	ctx = osdp_pd_setup(&info);
	if (ctx == NULL) {
		PyErr_SetString(PyExc_Exception, "failed to setup pd");
		goto error;
	}

	self->ctx = ctx;
	safe_free(channel_type_str);
	safe_free(device);
	safe_free(info.cap);
	return 0;
error:
	safe_free(channel_type_str);
	safe_free(device);
	safe_free(info.cap);
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
		pyosdp_pd_refresh_doc
	},
	{
		"set_command_callback",
		(PyCFunction)pyosdp_pd_set_command_callback,
		METH_VARARGS,
		pyosdp_pd_set_command_callback_doc
	},
	{
		"notify_event",
		(PyCFunction)pyosdp_pd_notify_event,
		METH_VARARGS,
		pyosdp_pd_notify_event_doc
	},
	{
		"sc_active",
		(PyCFunction)pyosdp_pd_is_sc_active,
		METH_NOARGS,
		pyosdp_pd_is_sc_active_doc
	},
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMemberDef pyosdp_pd_tp_members[] = {
	{ NULL } /* Sentinel */
};

PyTypeObject PeripheralDeviceTypeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	.tp_name      = "PeripheralDevice",
	.tp_basicsize = sizeof(pyosdp_t),
	.tp_itemsize  = 0,
	.tp_dealloc   = (destructor)pyosdp_pd_tp_dealloc,
	.tp_repr      = pyosdp_pd_tp_repr,
	.tp_str       = pyosdp_pd_tp_str,
	.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc       = pyosdp_pd_tp_init_doc,
	.tp_traverse  = (traverseproc)pyosdp_pd_tp_traverse,
	.tp_clear     = (inquiry)pyosdp_pd_tp_clear,
	.tp_methods   = pyosdp_pd_tp_methods,
	.tp_members   = pyosdp_pd_tp_members,
	.tp_getset    = pyosdp_pd_tp_getset,
	.tp_init      = (initproc)pyosdp_pd_tp_init,
	.tp_new       = pyosdp_pd_tp_new,
};

int pyosdp_add_type_pd(PyObject *module)
{
	return pyosdp_module_add_type(module, "PeripheralDevice",
				      &PeripheralDeviceTypeObject);
}
