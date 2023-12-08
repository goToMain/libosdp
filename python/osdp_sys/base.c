/*
 * Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module.h"

#define TAG "pyosdp_base"

int pyosdp_fops_open(void *arg, int file_id, int *size)
{
	int rc, ret;
	pyosdp_base_t *self = arg;
	PyObject *arglist, *result;

	if (!self->fops.open_cb)
		return -1;

	arglist = Py_BuildValue("(II)", file_id, *size);

	result = PyObject_CallObject(self->fops.open_cb, arglist);

	rc = pyosdp_parse_int(result, &ret);
	if (rc >= 0) {
		self->file_id = file_id;
		*size = ret;
		rc = 0;
	}

	Py_XDECREF(result);
	Py_DECREF(arglist);
	return rc;
}

int pyosdp_fops_read(void *arg, void *buf, int size, int offset)
{
	int rc, len = -1;
	uint8_t *rec_bytes;
	pyosdp_base_t *self = arg;
	PyObject *arglist, *bytes;

	if (!self->fops.read_cb)
		return -1;

	arglist = Py_BuildValue("(II)", size, offset);

	bytes = PyObject_CallObject(self->fops.read_cb, arglist);

	rc = pyosdp_parse_bytes(bytes, (uint8_t **)&rec_bytes, &len);
	if (rc == 0) {
		if (len <= size)
			memcpy(buf, rec_bytes, len);
		else
			len = -1;
	}

	Py_XDECREF(bytes);
	Py_DECREF(arglist);
	return len;
}

int pyosdp_fops_write(void *arg, const void *buf, int size, int offset)
{
	int written = 0;
	pyosdp_base_t *self = arg;
	PyObject *arglist, *result, *bytes;

	if (!self->fops.write_cb)
		return -1;

	bytes = Py_BuildValue("y#", buf, size);
	if (bytes == NULL)
		return -1;

	arglist = Py_BuildValue("(OI)", bytes, offset);
	result = PyObject_CallObject(self->fops.write_cb, arglist);

	pyosdp_parse_int(result, &written);

	Py_XDECREF(result);
	Py_DECREF(arglist);
	Py_DECREF(bytes);
	return written;
}

int  pyosdp_fops_close(void *arg)
{
	pyosdp_base_t *self = arg;
	PyObject *arglist, *result;

	if (!self->fops.close_cb)
		return 0;

	arglist = Py_BuildValue("(I)", self->file_id);

	result = PyObject_CallObject(self->fops.close_cb, arglist);

	Py_XDECREF(result);
	Py_DECREF(arglist);
	return 0;
}

#define pyosdp_file_tx_status_doc                                                     \
	"Get status of the current file transfer\n"                                   \
	"\n"                                                                          \
	"@return dictionary of keys 'size' and 'offset' if file TX is in progress.\n"
static PyObject *pyosdp_get_file_tx_status(pyosdp_base_t *self, PyObject *args)
{
	int pd_idx, size, offset;
	osdp_t *ctx;
	PyObject *dict;
	pyosdp_cp_t *cp = (pyosdp_cp_t *)self;
	pyosdp_pd_t *pd = (pyosdp_pd_t *)self;

	ctx = self->is_cp ? cp->ctx : pd->ctx;

	if (!PyArg_ParseTuple(args, "I", &pd_idx))
		Py_RETURN_NONE;

	if (osdp_get_file_tx_status(ctx, pd_idx, &size, &offset))
		Py_RETURN_NONE;

	dict = PyDict_New();
	if (dict == NULL)
		Py_RETURN_NONE;

	if (pyosdp_dict_add_int(dict, "size", size))
		Py_RETURN_NONE;

	if (pyosdp_dict_add_int(dict, "offset", offset))
		Py_RETURN_NONE;

	return dict;
}

#define pyosdp_file_register_ops_doc                                           \
	"Register file OPs handler\n"                                          \
	"\n"                                                                   \
	"@param struct osdp_file_ops dict. See osdp.h for details\n"           \
	"\n"                                                                   \
	"@return boolean status of command submission\n"
static PyObject *pyosdp_file_register_ops(pyosdp_base_t *self, PyObject *args)
{
	int rc, pd_idx;
	PyObject *fops_dict;
	osdp_t *ctx;
	pyosdp_cp_t *cp = (pyosdp_cp_t *)self;
	pyosdp_pd_t *pd = (pyosdp_pd_t *)self;

	if (!PyArg_ParseTuple(args, "IO!", &pd_idx, &PyDict_Type, &fops_dict))
		Py_RETURN_FALSE;

	if (self->is_cp) {
		if (pd_idx < 0 || pd_idx >= cp->num_pd) {
			PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
			Py_RETURN_FALSE;
		}
		ctx = cp->ctx;
	} else {
		if (pd_idx != 0) {
			PyErr_SetString(PyExc_ValueError, "Invalid PD offset");
			Py_RETURN_FALSE;
		}
		ctx = pd->ctx;
	}

	rc = 0;
	rc |= pyosdp_dict_get_object(fops_dict, "open", &self->fops.open_cb);
	rc |= pyosdp_dict_get_object(fops_dict, "read", &self->fops.read_cb);
	rc |= pyosdp_dict_get_object(fops_dict, "write", &self->fops.write_cb);
	rc |= pyosdp_dict_get_object(fops_dict, "close", &self->fops.close_cb);
	if (rc != 0) {
		PyErr_SetString(PyExc_ValueError, "fops dict parse error");
		Py_RETURN_FALSE;
	}

	Py_INCREF(self->fops.open_cb);
	Py_INCREF(self->fops.read_cb);
	Py_INCREF(self->fops.write_cb);
	Py_INCREF(self->fops.close_cb);

	struct osdp_file_ops pyosdp_fops = {
		.arg = (void *)self,
		.open = pyosdp_fops_open,
		.read = pyosdp_fops_read,
		.write = pyosdp_fops_write,
		.close = pyosdp_fops_close
	};

	if (osdp_file_register_ops(ctx, pd_idx, &pyosdp_fops)) {
		PyErr_SetString(PyExc_ValueError, "fops registration failed");
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

#define pyosdp_set_loglevel_doc                                                \
	"Set OSDP logging level\n"                                             \
	"\n"                                                                   \
	"@param log_level OSDP log level (0 to 7)\n"                           \
	"\n"                                                                   \
	"@return None"
static PyObject *pyosdp_set_loglevel(pyosdp_base_t *self, PyObject *args)
{
	int log_level;

	if (!PyArg_ParseTuple(args, "I", &log_level))
		return NULL;

	if (log_level < OSDP_LOG_EMERG ||
	    log_level > OSDP_LOG_MAX_LEVEL) {
		PyErr_SetString(PyExc_KeyError, "invalid log level");
		return NULL;
	}

	osdp_logger_init("pyosdp", log_level, NULL);

	Py_RETURN_NONE;
}

PyObject *pyosdp_get_version(pyosdp_base_t *self, PyObject *args)
{
	const char *version;
	PyObject *obj;

	version = osdp_get_version();
	obj = Py_BuildValue("s", version);
	if (obj == NULL)
		return NULL;
	Py_INCREF(obj);
	return obj;
}

PyObject *pyosdp_get_source_info(pyosdp_base_t *self, PyObject *args)
{
	const char *info;
	PyObject *obj;

	info = osdp_get_source_info();
	obj = Py_BuildValue("s", info);
	if (obj == NULL)
		return NULL;
	Py_INCREF(obj);
	return obj;
}

static void pyosdp_base_tp_dealloc(pyosdp_base_t *self)
{
	channel_manager_teardown(&self->channel_manager);
	Py_XDECREF(self->fops.open_cb);
	Py_XDECREF(self->fops.read_cb);
	Py_XDECREF(self->fops.write_cb);
	Py_XDECREF(self->fops.close_cb);
}

static int pyosdp_base_tp_init(pyosdp_base_t *self, PyObject *args, PyObject *kwds)
{
	self->fops.open_cb = NULL;
	self->fops.read_cb = NULL;
	self->fops.write_cb = NULL;
	self->fops.close_cb = NULL;

	channel_manager_init(&self->channel_manager);

	osdp_logger_init("pyosdp", OSDP_LOG_INFO, NULL);

	return 0;
}

static PyMethodDef pyosdp_base_methods[] = {
	{ "get_version", (PyCFunction)pyosdp_get_version, METH_NOARGS,
	  "Get OSDP version as string" },
	{ "get_source_info", (PyCFunction)pyosdp_get_source_info, METH_NOARGS,
	  "Get LibOSDP source info string" },
	{ "set_loglevel", (PyCFunction)pyosdp_set_loglevel, METH_VARARGS,
	  pyosdp_set_loglevel_doc },
	{ "register_file_ops", (PyCFunction)pyosdp_file_register_ops, METH_VARARGS,
	  pyosdp_file_register_ops_doc },
	{ "get_file_tx_status", (PyCFunction)pyosdp_get_file_tx_status, METH_VARARGS,
	  pyosdp_file_tx_status_doc },
	{ NULL } /* Sentinel */
};

PyTypeObject OSDPBaseType = {
	PyVarObject_HEAD_INIT(NULL, 0).tp_name = "osdp.Base",
	.tp_doc = "OSDP Base Class",
	.tp_basicsize = sizeof(pyosdp_base_t),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_init = (initproc)pyosdp_base_tp_init,
	.tp_dealloc = (destructor)pyosdp_base_tp_dealloc,
	.tp_methods = pyosdp_base_methods,
};

int pyosdp_add_type_osdp_base(PyObject *module)
{
	return pyosdp_module_add_type(module, "_OSDPBaseClass", &OSDPBaseType);
}
