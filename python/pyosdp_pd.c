/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

static const char pyosdp_pd_tp_doc[] =
"\n"
;

static int pyosdp_dict_add_int(PyObject *dict, const char *key, int val)
{
	int ret;
	PyObject *val_obj;

	val_obj = PyLong_FromLong((long)val);
	if (val_obj == NULL)
		return -1;
	ret = PyDict_SetItemString(dict, key, val_obj);
	Py_DECREF(val_obj);
	return ret;
}

static int pyosdp_dict_add_str(PyObject *dict, const char *key, const char *val)
{
	int ret;
	PyObject *val_obj;

	val_obj = PyUnicode_FromString(val);
	if (val_obj == NULL)
		return -1;
	ret = PyDict_SetItemString(dict, key, val_obj);
	Py_DECREF(val_obj);
	return ret;
}

PyObject *pyosdp_command_make_dict(struct osdp_cmd *cmd)
{
	char buf[64];
	int is_temporary = 0;
	PyObject *obj;
	struct osdp_cmd_led_params *p = &cmd->led.permanent;

	obj = PyDict_New();
	if (obj == NULL)
		return NULL;

	if (pyosdp_dict_add_int(obj, "command", cmd->id))
		return NULL;

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->output.control_code))
			return NULL;
		if (pyosdp_dict_add_int(obj, "output_no", cmd->output.output_no))
			return NULL;
		if (pyosdp_dict_add_int(obj, "timer_count", cmd->output.timer_count))
			return NULL;
		break;
	case OSDP_CMD_LED:
		if (cmd->led.temporary.control_code != 0) {
			p = &cmd->led.temporary;
			is_temporary = 1;
		}
		if (is_temporary) {
			if (pyosdp_dict_add_int(obj, "temporary", 1))
				return NULL;
		}
		if (pyosdp_dict_add_int(obj, "led_number", cmd->led.led_number))
			return NULL;
		if (pyosdp_dict_add_int(obj, "reader", cmd->led.reader))
			return NULL;
		if (pyosdp_dict_add_int(obj, "control_code", p->control_code))
			return NULL;
		if (pyosdp_dict_add_int(obj, "off_color", p->off_color))
			return NULL;
		if (pyosdp_dict_add_int(obj, "on_color", p->on_color))
			return NULL;
		if (pyosdp_dict_add_int(obj, "on_count", p->on_count))
			return NULL;
		if (pyosdp_dict_add_int(obj, "off_count", p->off_count))
			return NULL;
		if (pyosdp_dict_add_int(obj, "timer_count", p->timer_count))
			return NULL;
		break;
	case OSDP_CMD_BUZZER:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->buzzer.control_code))
			return NULL;
		if (pyosdp_dict_add_int(obj, "on_count", cmd->buzzer.on_count))
			return NULL;
		if (pyosdp_dict_add_int(obj, "off_count", cmd->buzzer.off_count))
			return NULL;
		if (pyosdp_dict_add_int(obj, "reader", cmd->buzzer.reader))
			return NULL;
		if (pyosdp_dict_add_int(obj, "rep_count", cmd->buzzer.rep_count))
			return NULL;
		break;
	case OSDP_CMD_TEXT:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->text.control_code))
			return NULL;
		if (pyosdp_dict_add_int(obj, "offset_col", cmd->text.offset_col))
			return NULL;
		if (pyosdp_dict_add_int(obj, "offset_row", cmd->text.offset_row))
			return NULL;
		if (pyosdp_dict_add_int(obj, "reader", cmd->text.reader))
			return NULL;
		if (pyosdp_dict_add_int(obj, "reader", cmd->text.reader))
			return NULL;
		if (cmd->text.length > (sizeof(buf)-1))
			return NULL;
		memcpy(buf, cmd->text.data, cmd->text.length);
		buf[cmd->text.length] = '\0';
		if (pyosdp_dict_add_str(obj, "data", buf))
			return NULL;
		break;
	case OSDP_CMD_KEYSET:
		if (pyosdp_dict_add_int(obj, "type", cmd->keyset.type))
			return NULL;
		if (cmd->keyset.length > 16)
			return NULL;
		if (atohstr(buf, cmd->keyset.data, cmd->keyset.length))
			return NULL;
		if (pyosdp_dict_add_str(obj, "data", buf))
			return NULL;
		break;
	case OSDP_CMD_COMSET:
		if (pyosdp_dict_add_int(obj, "address", cmd->comset.address))
			return NULL;
		if (pyosdp_dict_add_int(obj, "baud_rate", cmd->comset.baud_rate))
			return NULL;
		break;
	default:
		PyErr_SetString(PyExc_NotImplementedError,
				"command not implemented");
		return NULL;
	}

	return obj;
}

static int pd_command_cb(void *arg, int address, struct osdp_cmd *cmd)
{
	int ret_val = -1;
	pyosdp_t *self = arg;
	PyObject *dict, *arglist, *result;

	dict = pyosdp_command_make_dict(cmd);
	if (dict == NULL)
		return -1;

	arglist = Py_BuildValue("(IO)", address, dict);
	result = PyEval_CallObject(self->command_cb, arglist);

	if (result && PyLong_Check(result)) {
		ret_val = (int)PyLong_AsLong(result);
	}

	Py_XDECREF(dict);
	Py_XDECREF(result);
	Py_DECREF(arglist);
	return ret_val;
}

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
	osdp_pd_set_command_callback(self->ctx, pd_command_cb, (void *)self);
	Py_RETURN_NONE;
}

static PyObject *pyosdp_pd_get_command(pyosdp_t *self, PyObject *args)
{
	struct osdp_cmd cmd;

	if (osdp_pd_get_cmd(self->ctx, &cmd))
		Py_RETURN_NONE;

	return pyosdp_command_make_dict(&cmd);
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
		"Set loglevel"
	},
	{
		"get_command",
		(PyCFunction)pyosdp_pd_get_command,
		METH_VARARGS,
		"Get pending command"
	},
	{
		"set_command_callback",
		(PyCFunction)pyosdp_pd_set_command_callback,
		METH_VARARGS,
		"Set osdp command callback"
	},
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMemberDef pyosdp_pd_tp_members[] = {
	{ NULL } /* Sentinel */
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
