/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

static const char pyosdp_cp_tp_doc[] =
"\n"
;

int pyosdp_cp_keypress_cb(void *data, int address, uint8_t key)
{
	int ret_val = -1;
	pyosdp_t *self = data;
	PyObject *arglist, *result;

	arglist = Py_BuildValue("(II)", address, (int)key);

	result = PyEval_CallObject(self->cardread_cb, arglist);

	if (result && PyLong_Check(result)) {
		ret_val = (int)PyLong_AsLong(result);
	}
	Py_XDECREF(result);
	Py_DECREF(arglist);

	return ret_val;
}

int pyosdp_cp_cardread_cb(void *data, int address, int format, uint8_t *card_data, int len)
{
	int i, ret_val = -1;
	pyosdp_t *self = data;
	PyObject *arglist, *result, *py_list, *tmp;

	py_list = PyList_New(len);
	for (i = 0; i < len; i++) {
		tmp = PyLong_FromLong((long)card_data[i]);
		PyList_SetItem(py_list, i, tmp);
	}
	arglist = Py_BuildValue("(IIO)", address, format, py_list);

	result = PyEval_CallObject(self->keypress_cb, arglist);

	if (result && PyLong_Check(result)) {
		ret_val = (int)PyLong_AsLong(result);
	}
	Py_XDECREF(result);
	Py_DECREF(arglist);

	return ret_val;
}

static PyObject *pyosdp_cp_set_callback(pyosdp_t *self, PyObject *args)
{
	PyObject *dict, *keypress_cb = NULL, *cardread_cb = NULL;

	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;

	keypress_cb = PyDict_GetItemString(dict, "keypress");
	cardread_cb = PyDict_GetItemString(dict, "cardread");

	if ((keypress_cb && !PyCallable_Check(keypress_cb)) ||
	    (cardread_cb && !PyCallable_Check(cardread_cb))) {
		PyErr_SetString(PyExc_TypeError, "Need a callable object!");
		return NULL;
	}

	if (keypress_cb) self->keypress_cb = keypress_cb;
	if (cardread_cb) self->cardread_cb = cardread_cb;

	Py_RETURN_NONE;
}

static PyObject *pyosdp_cp_set_loglevel(pyosdp_t *self, PyObject *args)
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

static PyObject *pyosdp_cp_refresh(pyosdp_t *self, pyosdp_t *args)
{
	osdp_cp_refresh(self->ctx);

	Py_RETURN_NONE;
}

static int pyosdp_handle_cmd_output(pyosdp_t *self, int pd, PyObject *dict)
{
	struct osdp_cmd_output cmd;
	int output_no, control_code, timer_count;

	if (pyosdp_dict_get_int(dict, "output_no", &output_no))
		return -1;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		return -1;

	if (pyosdp_dict_get_int(dict, "timer_count", &timer_count))
		return -1;

	cmd.output_no = (uint8_t)output_no;
	cmd.control_code = (uint8_t)control_code;
	cmd.timer_count = (uint8_t)timer_count;

	if (osdp_cp_send_cmd_output(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_output enqueue failed");
		return -1;
	}

	return 0;
}

static int pyosdp_handle_cmd_led(pyosdp_t *self, int pd, PyObject *dict)
{
	int led_number, reader, off_color, on_color,
	    off_count, on_count, timer_count, control_code;
	struct osdp_cmd_led cmd;
	struct osdp_cmd_led_params *p = &cmd.permanent;

	if (pyosdp_dict_get_int(dict, "led_number", &led_number))
		return -1;

	if (pyosdp_dict_get_int(dict, "reader", &reader))
		return -1;

	if (PyDict_GetItemString(dict, "temporary"))
		p = &cmd.temporary;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		return -1;

	if (pyosdp_dict_get_int(dict, "off_color", &off_color))
		return -1;

	if (pyosdp_dict_get_int(dict, "on_color", &on_color))
		return -1;

	if (pyosdp_dict_get_int(dict, "off_count", &off_count))
		return -1;

	if (pyosdp_dict_get_int(dict, "on_count", &on_count))
		return -1;

	if (pyosdp_dict_get_int(dict, "timer_count", &timer_count))
		return -1;

	cmd.led_number = (uint8_t)led_number;
	cmd.reader = (uint8_t)reader;
	p->control_code = (uint8_t)control_code;
	p->off_color = (uint8_t)off_color;
	p->on_color = (uint8_t)on_color;
	p->on_count = (uint8_t)on_count;
	p->off_count = (uint8_t)off_count;
	p->timer_count = (uint8_t)timer_count;

	if (osdp_cp_send_cmd_led(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_led enqueue failed");
		return -1;
	}
	return 0;
}

static int pyosdp_handle_cmd_buzzer(pyosdp_t *self, int pd, PyObject *dict)
{
	struct osdp_cmd_buzzer cmd;
	int reader, on_count, off_count, rep_count, control_code;

	if (pyosdp_dict_get_int(dict, "reader", &reader))
		return -1;

	if (pyosdp_dict_get_int(dict, "on_count", &on_count))
		return -1;

	if (pyosdp_dict_get_int(dict, "off_count", &off_count))
		return -1;

	if (pyosdp_dict_get_int(dict, "rep_count", &rep_count))
		return -1;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		return -1;

	cmd.reader = (uint8_t)reader;
	cmd.on_count = (uint8_t)on_count;
	cmd.off_count = (uint8_t)off_count;
	cmd.rep_count = (uint8_t)rep_count;
	cmd.control_code = (uint8_t)control_code;

	if (osdp_cp_send_cmd_buzzer(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_buzzer enqueue failed");
		return -1;
	}
	return 0;
}

static int pyosdp_handle_cmd_text(pyosdp_t *self, int pd, PyObject *dict)
{
	char *data = NULL;
	struct osdp_cmd_text cmd;
	int length, reader, control_code, offset_row, offset_col, temp_time;

	if (pyosdp_dict_get_str(dict, "data", &data))
		goto error;

	length = strlen(data);
	if (length > OSDP_CMD_TEXT_MAX_LEN)
		goto error;

	if (pyosdp_dict_get_int(dict, "reader", &reader))
		goto error;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		goto error;

	if (pyosdp_dict_get_int(dict, "offset_col", &offset_col))
		goto error;

	if (pyosdp_dict_get_int(dict, "offset_row", &offset_row))
		goto error;

	if (pyosdp_dict_get_int(dict, "temp_time", &temp_time))
		goto error;

	cmd.reader = (uint8_t)reader;
	cmd.control_code = (uint8_t)control_code;
	cmd.length = (uint8_t)length;
	cmd.offset_col = (uint8_t)offset_col;
	cmd.offset_row = (uint8_t)offset_row;
	cmd.temp_time = (uint8_t)temp_time;
	memcpy(cmd.data, data, length);

	if (osdp_cp_send_cmd_text(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_text enqueue failed");
		goto error;
	}

	safe_free(data);
	return 0;
error:
	safe_free(data);
	return -1;
}

static int pyosdp_handle_cmd_keyset(pyosdp_t *self, PyObject *dict)
{
	struct osdp_cmd_keyset cmd;
	int type;
	char *data = NULL;

	if (pyosdp_dict_get_int(dict, "type", &type))
		goto error;

	if (pyosdp_dict_get_str(dict, "data", &data))
		goto error;

	cmd.type = (uint8_t)type;
	cmd.length = strlen(data);
	if (cmd.length > (OSDP_CMD_KEYSET_KEY_MAX_LEN * 2))
		goto error;
	cmd.length = hstrtoa(cmd.data, data);
	if (cmd.length < 0)
		goto error;

	if (osdp_cp_send_cmd_keyset(self->ctx, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_keyset enqueue failed");
		goto error;
	}

	safe_free(data);
	return 0;
error:
	safe_free(data);
	return -1;
}

static int pyosdp_handle_cmd_comset(pyosdp_t *self, int pd, PyObject *dict)
{
	struct osdp_cmd_comset cmd;
	int address, baud_rate;

	if (pyosdp_dict_get_int(dict, "address", &address))
		return -1;

	if (pyosdp_dict_get_int(dict, "baud_rate", &baud_rate))
		return -1;

	cmd.address = (uint8_t)address;
	cmd.baud_rate = (uint32_t)baud_rate;

	if (osdp_cp_send_cmd_comset(self->ctx, pd, &cmd)) {
		PyErr_SetString(PyExc_KeyError, "cmd_comset enqueue failed");
		return -1;
	}
	return 0;
}

static PyObject *pyosdp_cp_send_command(pyosdp_t *self, PyObject *args)
{
	int pd, cmd_id;
	PyObject *cmd;
	if (!PyArg_ParseTuple(args, "IO!", &pd, &PyDict_Type, &cmd))
		return NULL;

	if (pd < 0 || pd >= self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		return NULL;
	}

	if (pyosdp_dict_get_int(cmd, "command", &cmd_id))
		return NULL;

	switch(cmd_id) {
	case OSDP_CMD_OUTPUT:
		if (pyosdp_handle_cmd_output(self, pd, cmd))
			return NULL;
		break;
	case OSDP_CMD_LED:
		if (pyosdp_handle_cmd_led(self, pd, cmd))
			return NULL;
		break;
	case OSDP_CMD_BUZZER:
		if (pyosdp_handle_cmd_buzzer(self, pd, cmd))
			return NULL;
		break;
	case OSDP_CMD_TEXT:
		if (pyosdp_handle_cmd_text(self, pd, cmd))
			return NULL;
		break;
	case OSDP_CMD_KEYSET:
		if (pyosdp_handle_cmd_keyset(self, cmd))
			return NULL;
		break;
	case OSDP_CMD_COMSET:
		if (pyosdp_handle_cmd_comset(self, pd, cmd))
			return NULL;
		break;
	default:
		PyErr_SetString(PyExc_NotImplementedError,
				"command not implemented");
		return NULL;
	}

	Py_RETURN_NONE;
}

static int pyosdp_cp_tp_clear(pyosdp_t *self)
{
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
	return (PyObject *)self;
}

static void pyosdp_cp_tp_dealloc(pyosdp_t *self)
{
	osdp_cp_teardown(self->ctx);
	channel_manager_teardown(&self->chn_mgr);
	pyosdp_cp_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static int pyosdp_cp_tp_init(pyosdp_t *self, PyObject *args, PyObject *kwargs)
{
	int i, ret=-1, tmp;
	uint8_t master_key[16];
	enum channel_type channel_type;
	PyObject *py_info_list, *py_info;
	static char *kwlist[] = { "", "master_key", NULL };
	char *device = NULL, *channel_type_str = NULL, *master_key_str = NULL;
	osdp_t *ctx;
	osdp_pd_info_t *info, *info_list = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|$s:cp_init", kwlist,
					 &PyList_Type, &py_info_list,
					 &master_key_str))
		goto error;

	if (master_key_str && (strlen(master_key_str) != 32 ||
	    hstrtoa(master_key, master_key_str) == -1)) {
		PyErr_SetString(PyExc_TypeError, "master_key parse error");
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
			PyErr_SetString(PyExc_ValueError, "py_info_list extract error");
			goto error;
		}

		if (pyosdp_dict_get_int(py_info, "address", &info->address))
			goto error;

		if (pyosdp_dict_get_int(py_info, "channel_speed", &info->baud_rate))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_type", &channel_type_str))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_device", &device))
			goto error;

		info->flags = 0;
		info->cap = NULL;

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
			    &info->channel.data,
			    &info->channel.send,
			    &info->channel.recv,
			    &info->channel.flush);
	}

	ctx = osdp_cp_setup(self->num_pd, info_list,
			    master_key_str ? master_key : NULL);
	if (ctx == NULL) {
		PyErr_SetString(PyExc_Exception, "failed to setup CP");
		goto error;
	}

	osdp_cp_set_callback_data(ctx, (void *)self);
	osdp_cp_set_callback_card_read(ctx, pyosdp_cp_cardread_cb);
	osdp_cp_set_callback_key_press(ctx, pyosdp_cp_keypress_cb);

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
		"Periodic refresh hook"
	},
	{
		"set_callback",
		(PyCFunction)pyosdp_cp_set_callback,
		METH_VARARGS,
		"Set osdp event callbacks"
	},
	{
		"send",
		(PyCFunction)pyosdp_cp_send_command,
		METH_VARARGS,
		"Send osdp commands"
	},
	{
		"set_loglevel",
		(PyCFunction)pyosdp_cp_set_loglevel,
		METH_VARARGS,
		"Set osdp event callbacks"
	},
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMemberDef pyosdp_cp_tp_members[] = {
	{ NULL } /* Sentinel */
};

PyTypeObject ControlPanelTypeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"ControlPanel",					/* tp_name */
	sizeof(pyosdp_t),				/* tp_basicsize */
	0,						/* tp_itemsize */
	(destructor)pyosdp_cp_tp_dealloc,		/* tp_dealloc */
	0,						/* tp_print */
	0,						/* tp_getattr */
	0,						/* tp_setattr */
	0,						/* tp_compare */
	pyosdp_cp_tp_repr,				/* tp_repr */
	0,						/* tp_as_number */
	0,						/* tp_as_sequence */
	0,						/* tp_as_mapping */
	0,						/* tp_hash */
	0,						/* tp_call */
	pyosdp_cp_tp_str,				/* tp_str */
	0,						/* tp_getattro */
	0,						/* tp_setattro */
	0,						/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
	pyosdp_cp_tp_doc,				/* tp_doc */
	(traverseproc)pyosdp_cp_tp_traverse,		/* tp_traverse */
	(inquiry)pyosdp_cp_tp_clear,			/* tp_clear */
	0,						/* tp_richcompare */
	0,						/* tp_weaklistoffset */
	0,						/* tp_iter */
	0,						/* tp_iternext */
	pyosdp_cp_tp_methods,				/* tp_methods */
	pyosdp_cp_tp_members,				/* tp_members */
	pyosdp_cp_tp_getset,				/* tp_getset */
	0,						/* tp_base */
	0,						/* tp_dict */
	0,						/* tp_descr_get */
	0,						/* tp_descr_set */
	0,						/* tp_dictoffset */
	(initproc)pyosdp_cp_tp_init,			/* tp_init */
	0,						/* tp_alloc */
	pyosdp_cp_tp_new,				/* tp_new */
	0,						/* tp_free */
	0						/* tp_is_gc */
};

int pyosdp_add_type_cp(PyObject *module)
{
	return pyosdp_module_add_type(module, "ControlPanel",
				      &ControlPanelTypeObject);
}
