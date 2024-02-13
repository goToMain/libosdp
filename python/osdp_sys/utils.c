/*
 * Copyright (c) 2020-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module.h"

int pyosdp_dict_add_bool(PyObject *dict, const char *key, bool val)
{
	int ret;
	PyObject *val_obj;

	val_obj = val ? Py_True : Py_False;

	Py_INCREF(val_obj);
	ret = PyDict_SetItemString(dict, key, val_obj);
	Py_DECREF(val_obj);
	return ret;
}

int pyosdp_dict_add_int(PyObject *dict, const char *key, int val)
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

int pyosdp_dict_add_str(PyObject *dict, const char *key, const char *val)
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

int pyosdp_dict_add_bytes(PyObject *dict, const char *key, const uint8_t *data,
			  int len)
{
	int ret;
	PyObject *obj;

	obj = Py_BuildValue("y#", data, len);
	if (obj == NULL)
		return -1;
	ret = PyDict_SetItemString(dict, key, obj);
	Py_DECREF(obj);
	return ret;
}

int pyosdp_module_add_type(PyObject *module, const char *name,
			   PyTypeObject *type)
{
	if (PyType_Ready(type)) {
		return -1;
	}
	Py_INCREF(type);
	if (PyModule_AddObject(module, name, (PyObject *)type)) {
		Py_DECREF(type);
		return -1;
	}
	return 0;
}

int pyosdp_parse_int(PyObject *obj, int *res)
{
	PyObject *tmp;

	/* Check if obj is numeric */
	if (PyNumber_Check(obj) != 1) {
		PyErr_SetString(PyExc_TypeError, "Expected number");
		return -1;
	}

	tmp = PyNumber_Long(obj);
	*res = (int)PyLong_AsUnsignedLong(tmp);
	Py_DECREF(tmp);
	return 0;
}

int pyosdp_parse_bool(PyObject *obj, bool *res)
{
	/* Check if obj is boolean */
	if (PyBool_Check(obj) != 1) {
		PyErr_SetString(PyExc_TypeError, "Expected boolean");
		return -1;
	}

	*res = PyObject_IsTrue(obj);
	return 0;
}

int pyosdp_parse_str(PyObject *obj, char **str)
{
	char *s;
	PyObject *str_ref;

	str_ref = PyUnicode_AsEncodedString(obj, "UTF-8", "strict");
	if (str_ref == NULL) {
		PyErr_SetString(PyExc_TypeError, "Expected string");
		return -1;
	}

	s = PyBytes_AsString(str_ref);
	if (s == NULL) {
		PyErr_SetString(PyExc_TypeError, "Expected string");
		Py_DECREF(str_ref);
		return -1;
	}
	*str = strdup(s);
	Py_DECREF(str_ref);
	return 0;
}

int pyosdp_parse_bytes(PyObject *obj, uint8_t **data, int *length)
{
	Py_ssize_t len;
	uint8_t *buf;

	if (!obj || !PyArg_Parse(obj, "y#", &buf, &len))
		return -1;

	if (buf == NULL || len == 0) {
		PyErr_Format(PyExc_ValueError, "Unable to extact data bytes");
		return -1;
	}
	*data = buf;
	*length = len;
	return 0;
}

int pyosdp_dict_get_str(PyObject *dict, const char *key, char **str)
{
	PyObject *tmp;

	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	tmp = PyDict_GetItemString(dict, key);
	if (tmp == NULL) {
		PyErr_Format(PyExc_KeyError,
			     "Key: '%s' of type: string expected", key);
		return -1;
	}

	return pyosdp_parse_str(tmp, str);
}

int pyosdp_dict_get_int(PyObject *dict, const char *key, int *res)
{
	PyObject *tmp;

	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	tmp = PyDict_GetItemString(dict, key);
	if (tmp == NULL) {
		PyErr_Format(PyExc_KeyError, "Key: '%s' of type: int expected",
			     key);
		return -1;
	}

	return pyosdp_parse_int(tmp, res);
}

int pyosdp_dict_get_bool(PyObject *dict, const char *key, bool *res)
{
	PyObject *tmp;

	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	tmp = PyDict_GetItemString(dict, key);
	if (tmp == NULL)
		return 1;

	return pyosdp_parse_bool(tmp, res);
}

int pyosdp_dict_get_bytes(PyObject *dict, const char *key, uint8_t **data,
			  int *length)
{
	PyObject *obj;

	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	obj = PyDict_GetItemString(dict, key);
	if (obj == NULL) {
		PyErr_Format(PyExc_KeyError,
			     "Key: '%s' of type: bytes expected", key);
		return -1;
	}

	return pyosdp_parse_bytes(obj, data, length);
}

int pyosdp_dict_get_object(PyObject *dict, const char *key, PyObject **obj)
{
	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	*obj = PyDict_GetItemString(dict, key);
	if (*obj == NULL)
		return -1;

	return 0;
}
