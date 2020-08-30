/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/strutils.h>

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

static PyObject *pyosdp_cp_set_callback(pyosdp_t *self, PyObject *args,
					PyObject *kwargs)
{
	PyObject *keypress_cb = NULL, *cardread_cb = NULL;
	static char *kwlist[] = {
		"keypress", "cardread", NULL
	};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "$OO", kwlist,
					 &keypress_cb, &cardread_cb))
		return NULL;

	if ((keypress_cb && !PyCallable_Check(keypress_cb)) ||
	    (cardread_cb && !PyCallable_Check(cardread_cb))) {
		PyErr_SetString(PyExc_TypeError, "Need a callable object!");
		return NULL;
	}

	if (cardread_cb)
		self->cardread_cb = cardread_cb;
	if (keypress_cb)
		self->keypress_cb = keypress_cb;

	return Py_None;
}

static PyObject *pyosdp_cp_refresh(pyosdp_t *self, pyosdp_t *args)
{
	osdp_cp_refresh(self->ctx);
	return Py_None;
}

static int pyosdp_handle_cmd_output(pyosdp_t *self, int pd, int output_no,
				    int control_code, int tmr_count)
{
	struct osdp_cmd_output cmd;

	cmd.output_no = output_no;
	cmd.control_code = control_code;
	cmd.tmr_count = tmr_count;
	return osdp_cp_send_cmd_output(self->ctx, pd, &cmd);
}

static int pyosdp_handle_cmd_led(pyosdp_t *self, int pd, int control_code,
				 int on_count, int off_count, int on_color,
				 int off_color, int timer, int reader,
				 int led_number, int is_permanent)
{
	struct osdp_cmd_led cmd;
	struct osdp_cmd_led_params *p;

	cmd.led_number = led_number;
	cmd.reader = reader;

	p = (is_permanent) ? &cmd.permanent : &cmd.temporary;
	p->control_code = control_code;
	p->off_color = off_color;
	p->on_color = on_color;
	p->off_count = off_count;
	p->on_count = on_count;
	p->timer = timer;

	return osdp_cp_send_cmd_led(self->ctx, pd, &cmd);
}

static int pyosdp_handle_cmd_buzzer(pyosdp_t *self, int pd, int reader,
				    int control_code, int on_count, int off_count,
				    int rep_count)
{
	struct osdp_cmd_buzzer cmd;

	cmd.reader = reader;
	cmd.on_count = on_count;
	cmd.off_count = off_count;
	cmd.rep_count = rep_count;
	cmd.tone_code = control_code;

	return osdp_cp_send_cmd_buzzer(self->ctx, pd, &cmd);
}

static int pyosdp_handle_cmd_text(pyosdp_t *self, int pd, int reader,
				  int control_code, int temp_time,
				  int offset_row, int offset_col,
				  const char *data)
{
	int length;
	struct osdp_cmd_text cmd;

	length = strlen(data);
	if (length > OSDP_CMD_TEXT_MAX_LEN)
		return -1;

	cmd.reader = reader;
	cmd.cmd = control_code;
	cmd.length = length;
	cmd.offset_col = offset_col;
	cmd.offset_row = offset_row;
	cmd.temp_time = temp_time;
	memcpy(cmd.data, data, length);

	return osdp_cp_send_cmd_text(self->ctx, pd, &cmd);
}

static int pyosdp_handle_cmd_keyset(pyosdp_t *self, int key_type,
				    const char *key_data)
{
	struct osdp_cmd_keyset cmd;

	cmd.key_type = key_type;
	cmd.len = strlen(key_data);
	if (cmd.len > (OSDP_CMD_KEYSET_KEY_MAX_LEN * 2))
		return -1;
	cmd.len = hstrtoa(cmd.data, key_data);
	if (cmd.len < 0)
		return -1;

	return osdp_cp_send_cmd_keyset(self->ctx, &cmd);
}

static int pyosdp_handle_cmd_comset(pyosdp_t *self, int pd, int address,
				    int baud_rate)
{
	struct osdp_cmd_comset cmd;

	cmd.addr = address;
	cmd.baud = baud_rate;

	return osdp_cp_send_cmd_comset(self->ctx, pd, &cmd);
}

static PyObject *pyosdp_cp_send_command(pyosdp_t *self, PyObject *args,
					PyObject *kwargs)
{
	char *kwlist[] = {
	/* common */ "pd", "command", "reader", "number", "control_code",
	/* LED */    "on_color", "off_color", "timer", "temporary", "permanent",
	/* Buzzer */ "on_count", "off_count", "rep_count",
	/* Text */   "row", "col",
	/* comset */ "address", "baud_rate",
	/* Keyset */ "type", "data", NULL
	};

	/* required arguments */
	int command=-1, pd=-1;

	/* optional arguments */
	int reader=-1, number=-1, control_code=-1,
	on_color=-1, off_color=-1, timer=-1, temporary=-1, permanent=-1,
	on_count=-1, off_count=-1, rep_count=-1,
	row=-1, col=-1, address=-1, baud_rate=-1, type=-1;

	char *data=NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "II|IIIIIIIIIIIIIIIIs", kwlist,
					 &pd, &command, &reader, &number, &control_code,
					 &on_color, &off_color, &timer, &temporary, &permanent,
					 &on_count, &off_count, &rep_count,
					 &row, &col, &address, &baud_rate, &type,
					 &data))
		return NULL;

	if (pd < 0 || pd >= self->num_pd)
		return NULL;

	switch(command) {
	case OSDP_CMD_OUTPUT:
		pyosdp_handle_cmd_output(self, pd, number, control_code, timer);
		break;
	case OSDP_CMD_LED:
		pyosdp_handle_cmd_led(self, pd, control_code, on_count, off_count,
				      on_color, off_color, timer, reader, number,
				      (permanent || !temporary));
		break;
	case OSDP_CMD_BUZZER:
		pyosdp_handle_cmd_buzzer(self, pd, reader, control_code, on_count,
					 off_count, rep_count);
		break;
	case OSDP_CMD_TEXT:
		pyosdp_handle_cmd_text(self, pd, reader, control_code, timer,
				       row, col, data);
		break;
	case OSDP_CMD_KEYSET:
		pyosdp_handle_cmd_keyset(self, type, data);
		break;
	case OSDP_CMD_COMSET:
		pyosdp_handle_cmd_comset(self, pd, address, baud_rate);
		break;
	default:
		return NULL;
	}

	return Py_None;
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

int pyosdp_parse_int(PyObject *obj, int *res)
{
	PyObject *tmp;
	/* Check if obj is numeric */
	if (PyNumber_Check(obj) != 1) {
		PyErr_SetString(PyExc_TypeError, "Non-numeric argument.");
		return -1;
	}
	tmp = PyNumber_Long(obj);
	*res = (int)PyLong_AsUnsignedLong(tmp);
	Py_DECREF(tmp);
	return 0;
}

static int pyosdp_cp_tp_init(pyosdp_t *self, PyObject *args, PyObject *kwargs)
{
	enum channel_type channel_type;
	int i, ret, address, baud_rate;
	char *device, *channel_type_str, *master_key_str = NULL;
	osdp_pd_info_t *info, *info_list = NULL;
	PyObject *py_info_list, *py_info;
	uint8_t master_key[16] = {};
	osdp_t *ctx;
	static char *kwlist[] = {
		"pd_info", "master_key", NULL
	};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|$s", kwlist,
					 &PyList_Type, &py_info_list,
					 &master_key_str))
		return -1;

	if (master_key_str) {
		if (strlen(master_key_str) != 32 ||
		    hstrtoa(master_key, master_key_str) == -1) {
			PyErr_SetString(PyExc_TypeError, "master_key parse error");
			return -1;
		}
	}

	channel_manager_init(&self->chn_mgr);

	self->num_pd = (int)PyList_Size(py_info_list);
	if (self->num_pd == 0)
		return -1;
	info_list = calloc(self->num_pd, sizeof(osdp_pd_info_t));
	if (info_list == NULL) {
		PyErr_SetString(PyExc_MemoryError, "pd_info alloc error");
		return -1;
	}
	for (i = 0; i < self->num_pd; i++) {
		py_info = PyList_GetItem(py_info_list, i);
		info = info_list + i;
		if (!PyArg_ParseTuple(py_info, "isis", &address,
				      &channel_type_str, &baud_rate, &device))
			goto error;

		info->flags = 0;
		info->address = address;
		info->baud_rate = baud_rate;
		info->cap = NULL;

		channel_type = channel_guess_type(channel_type_str);
		if (channel_type == CHANNEL_TYPE_ERR) {
			PyErr_SetString(PyExc_ValueError,
					"unable to guess channel type");
			goto error;
		}

		ret = channel_open(&self->chn_mgr, channel_type, device,
				   baud_rate, 0);
		if (ret != CHANNEL_ERR_NONE &&
		    ret != CHANNEL_ERR_ALREADY_OPEN) {
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

	self->ctx = ctx;
	free(info_list);
	return 0;

error:
	free(info_list);
	return -1;
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
		"Periodic refresh hook" },
	{
		"set_callback",
		(PyCFunction)pyosdp_cp_set_callback,
		METH_VARARGS|METH_KEYWORDS,
	  	"Set osdp event callbacks"
	},
	{
		"send",
		(PyCFunction)pyosdp_cp_send_command,
		METH_VARARGS|METH_KEYWORDS,
		"Send osdp commands"
	},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

static PyMemberDef pyosdp_cp_tp_members[] = {
	{NULL} /* Sentinel */
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
