/*
 * Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module.h"

#define TAG "pyosdp_cp"

#define pyosdp_cp_pd_status_doc                                                \
	"Get PD status, (online/offline) as a bitmask for all connected PDs\n" \
	"\n"                                                                   \
	"@return PD status bitmask"
static PyObject *pyosdp_cp_pd_status(pyosdp_cp_t *self, PyObject *args)
{
	uint32_t bitmask = 0;

	osdp_get_status_mask(self->ctx, (uint8_t *)&bitmask);

	return Py_BuildValue("I", bitmask);
}

#define pyosdp_cp_sc_status_doc                                                \
	"Get PD Secure Channel status bitmask of all connected PDs\n"          \
	"\n"                                                                   \
	"@return Secure Channel Status bitmask"
static PyObject *pyosdp_cp_sc_status(pyosdp_cp_t *self, PyObject *args)
{
	uint32_t bitmask = 0;

	osdp_get_sc_status_mask(self->ctx, (uint8_t *)&bitmask);

	return Py_BuildValue("I", bitmask);
}

int pyosdp_cp_event_cb(void *data, int address, struct osdp_event *event)
{
	pyosdp_cp_t *self = data;
	PyObject *arglist, *result, *event_dict;

	if (pyosdp_make_dict_event(&event_dict, event))
		return -1;

	arglist = Py_BuildValue("(IO)", address, event_dict);

	result = PyObject_CallObject(self->event_cb, arglist);

	Py_XDECREF(result);
	Py_DECREF(arglist);
	return 0;
}

#define pyosdp_cp_set_event_callback_doc                                       \
	"Set OSDP event callback handler\n"                                    \
	"\n"                                                                   \
	"@param callback A function to call when a PD reports an event\n"      \
	"\n"                                                                   \
	"@return None"
static PyObject *pyosdp_cp_set_event_callback(pyosdp_cp_t *self, PyObject *args)
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

#define pyosdp_cp_refresh_doc                                                   \
	"OSDP periodic refresh hook. Must be called at least once every 50ms\n" \
	"\n"                                                                    \
	"@return None\n"
static PyObject *pyosdp_cp_refresh(pyosdp_cp_t *self, pyosdp_cp_t *args)
{
	osdp_cp_refresh(self->ctx);

	Py_RETURN_NONE;
}

#define pyosdp_cp_send_command_doc                                                   \
	"Send an OSDP command to a PD\n"                                             \
	"\n"                                                                         \
	"@param pd PD offset number\n"                                               \
	"@param command A dict of command keys and values. See osdp.h for details\n" \
	"\n"                                                                         \
	"@return boolean status of command submission\n"
static PyObject *pyosdp_cp_send_command(pyosdp_cp_t *self, PyObject *args)
{
	int pd, ret;
	PyObject *cmd_dict;
	struct osdp_cmd cmd;

	if (!PyArg_ParseTuple(args, "IO!", &pd, &PyDict_Type, &cmd_dict))
		Py_RETURN_FALSE;

	if (pd < 0 || pd >= self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		Py_RETURN_FALSE;
	}

	memset(&cmd, 0, sizeof(struct osdp_cmd));
	if (pyosdp_make_struct_cmd(&cmd, cmd_dict))
		Py_RETURN_FALSE;

	ret = osdp_cp_send_command(self->ctx, pd, &cmd);

	if (ret == 0)
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

static PyObject *pyosdp_cp_modify_flag(pyosdp_cp_t *self, PyObject *args, bool do_set)
{
	int ret, pd, flags;

	if (!PyArg_ParseTuple(args, "II", &pd, &flags))
		Py_RETURN_FALSE;

	if (pd < 0 || pd >= self->num_pd) {
		PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
		Py_RETURN_FALSE;
	}

	ret = osdp_cp_modify_flag(self->ctx, pd, (uint32_t)flags, do_set);

	if (ret == 0)
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

#define pyosdp_cp_set_flag_doc \
	"Set PD flag\n" \
	"\n" \
	"@param pd_idx PD offset number\n" \
	"@param flag One of the OSDP public flags" \
	"\n" \
	"@return boolean status"
static PyObject *pyosdp_cp_set_flag(pyosdp_cp_t *self, PyObject *args)
{
	return pyosdp_cp_modify_flag(self, args, true);
}

#define pyosdp_cp_clear_flag_doc \
	"Clear PD flag\n" \
	"\n" \
	"@param pd_idx PD offset number\n" \
	"@param flag One of the OSDP public flags" \
	"\n" \
	"@return boolean status"
static PyObject *pyosdp_cp_clear_flag(pyosdp_cp_t *self, PyObject *args)
{
	return pyosdp_cp_modify_flag(self, args, false);
}

static int pyosdp_cp_tp_clear(pyosdp_cp_t *self)
{
	Py_XDECREF(self->event_cb);
	return 0;
}

static PyObject *pyosdp_cp_tp_new(PyTypeObject *type, PyObject *args,
				  PyObject *kwargs)
{
	pyosdp_cp_t *self = NULL;

	self = (pyosdp_cp_t *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}
	self->ctx = NULL;
	self->event_cb = NULL;
	self->num_pd = 0;
	return (PyObject *)self;
}

static void pyosdp_cp_tp_dealloc(pyosdp_cp_t *self)
{
	if (self->ctx)
		osdp_cp_teardown(self->ctx);

	/* call base class destructor */
	OSDPBaseType.tp_dealloc((PyObject *)self);

	pyosdp_cp_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

#define pyosdp_cp_tp_init_doc                                                                \
	"OSDP Control Panel Class\n"                                                         \
	"\n"                                                                                 \
	"@param pd_info List of PD info dicts. See osdp_pd_info_t in osdp.h for more info\n" \
	"@param master_key A hexadecimal string representation of the master key\n"          \
	"\n"                                                                                 \
	"@return None"
static int pyosdp_cp_tp_init(pyosdp_cp_t *self, PyObject *args, PyObject *kwargs)
{
	int i, ret = -1, len;
	uint8_t *scbk = NULL;
	enum channel_type channel_type;
	PyObject *py_info_list, *py_info;
	static char *kwlist[] = { "", NULL };
	char *device = NULL, *channel_type_str = NULL;
	osdp_t *ctx;
	osdp_pd_info_t *info, *info_list = NULL;

	/* call base class constructor */
	if (OSDPBaseType.tp_init((PyObject *)self, args, kwargs) < 0)
		return -1;

	self->base.is_cp = true;

	/* the string after the : is used as the function name in error messages */
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!:pyosdp_cp_init",
					 kwlist, &PyList_Type, &py_info_list))
		goto error;

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

		pyosdp_dict_get_str(py_info, "name", &self->name);
		info->name = self->name;

		if (pyosdp_dict_get_int(py_info, "address", &info->address))
			goto error;

		if (pyosdp_dict_get_int(py_info, "flags", &info->flags))
			goto error;

		if (pyosdp_dict_get_int(py_info, "channel_speed",
					&info->baud_rate))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_type",
					&channel_type_str))
			goto error;

		if (pyosdp_dict_get_str(py_info, "channel_device", &device))
			goto error;

		if (pyosdp_dict_get_bytes(py_info, "scbk", &scbk, &len) == 0) {
			if (scbk && len != 16) {
				PyErr_SetString(PyExc_TypeError,
						"scbk must be exactly 16 bytes");
				goto error;
			}
			info->scbk = scbk;
		}
		PyErr_Clear();

		channel_type = channel_guess_type(channel_type_str);
		if (channel_type == CHANNEL_TYPE_ERR) {
			PyErr_SetString(PyExc_ValueError,
					"unable to guess channel type");
			goto error;
		}

		ret = channel_open(&self->base.channel_manager, channel_type,
				   device, info->baud_rate, 0);
		if (ret != CHANNEL_ERR_NONE &&
		    ret != CHANNEL_ERR_ALREADY_OPEN) {
			PyErr_SetString(PyExc_PermissionError,
					"Unable to open channel");
			goto error;
		}
		channel_get(&self->base.channel_manager, device, &info->channel.id,
			    &info->channel.data, &info->channel.send,
			    &info->channel.recv, &info->channel.flush);
	}

	ctx = osdp_cp_setup(self->num_pd, info_list);
	if (ctx == NULL) {
		PyErr_SetString(PyExc_Exception, "failed to setup CP");
		goto error;
	}

	osdp_cp_set_event_callback(ctx, pyosdp_cp_event_cb, self);

	self->ctx = ctx;
	return 0;
error:
	safe_free(info_list);
	safe_free(channel_type_str);
	safe_free(device);
	return -1;
}

PyObject *pyosdp_cp_tp_repr(PyObject *self)
{
	PyObject *py_string;

	py_string = Py_BuildValue("s", "control panel object");
	Py_INCREF(py_string);

	return py_string;
}

static int pyosdp_cp_tp_traverse(pyosdp_cp_t *self, visitproc visit, void *arg)
{
	Py_VISIT(self->ctx);
	return 0;
}

PyObject *pyosdp_cp_tp_str(PyObject *self)
{
	return pyosdp_cp_tp_repr(self);
}

static PyGetSetDef pyosdp_cp_tp_getset[] = {
	{ NULL } /* Sentinel */
};

static PyMethodDef pyosdp_cp_tp_methods[] = {
	{ "refresh", (PyCFunction)pyosdp_cp_refresh,
	  METH_NOARGS, pyosdp_cp_refresh_doc },
	{ "set_event_callback", (PyCFunction)pyosdp_cp_set_event_callback,
	  METH_VARARGS, pyosdp_cp_set_event_callback_doc },
	{ "send_command", (PyCFunction)pyosdp_cp_send_command,
	  METH_VARARGS, pyosdp_cp_send_command_doc },
	{ "status", (PyCFunction)pyosdp_cp_pd_status,
	  METH_NOARGS, pyosdp_cp_pd_status_doc },
	{ "sc_status", (PyCFunction)pyosdp_cp_sc_status,
	  METH_NOARGS, pyosdp_cp_sc_status_doc },
	{ "set_flag", (PyCFunction)pyosdp_cp_set_flag,
	  METH_NOARGS, pyosdp_cp_set_flag_doc },
	{ "clear_flag", (PyCFunction)pyosdp_cp_clear_flag,
	  METH_NOARGS, pyosdp_cp_clear_flag_doc },
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMemberDef pyosdp_cp_tp_members[] = {
	{ NULL } /* Sentinel */
};

static PyTypeObject ControlPanelTypeObject = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0).tp_name = "ControlPanel",
	.tp_doc = pyosdp_cp_tp_init_doc,
	.tp_basicsize = sizeof(pyosdp_cp_t),
	.tp_itemsize = 0,
	.tp_dealloc = (destructor)pyosdp_cp_tp_dealloc,
	.tp_repr = pyosdp_cp_tp_repr,
	.tp_str = pyosdp_cp_tp_str,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_traverse = (traverseproc)pyosdp_cp_tp_traverse,
	.tp_clear = (inquiry)pyosdp_cp_tp_clear,
	.tp_methods = pyosdp_cp_tp_methods,
	.tp_members = pyosdp_cp_tp_members,
	.tp_getset = pyosdp_cp_tp_getset,
	.tp_init = (initproc)pyosdp_cp_tp_init,
	.tp_new = pyosdp_cp_tp_new,
	.tp_base = &OSDPBaseType,
};

int pyosdp_add_type_cp(PyObject *module)
{
	return pyosdp_module_add_type(module, "ControlPanel",
				      &ControlPanelTypeObject);
}
