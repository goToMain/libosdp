/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

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

int pyosdp_dict_get_str(PyObject *dict, const char *key, char **str)
{
	PyObject *tmp;

	if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError, "arg is not a dict");
		return -1;
	}

	tmp = PyDict_GetItemString(dict, key);
	if (tmp == NULL) {
		PyErr_Format(PyExc_KeyError, "Key '%s' not found", key);
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
		PyErr_Format(PyExc_KeyError, "Key '%s' not found", key);
		return -1;
	}

	return pyosdp_parse_int(tmp, res);
}

static PyMethodDef osdp_funcs[] = {
	{ NULL, NULL, 0, NULL } /* sentinel */
};

static PyModuleDef osdp_module = {
	PyModuleDef_HEAD_INIT,
	"osdp", /* Module name */
	"Open Supervised Device Protocol", /* Docstring for the module */
	-1,   /* Optional size of the module state memory */
	osdp_funcs, /* pointer to a table of module-level functions */
	NULL, /* Optional slot definitions */
	NULL, /* Optional traversal function */
	NULL, /* Optional clear function */
	NULL  /* Optional module deallocation function */
};

void pyosdp_add_module_constants(PyObject *module)
{
#define ADD_CONST(k,v) PyModule_AddIntConstant(module, k, v);

	/* enum osdp_cmd_e */
	ADD_CONST("CMD_OUTPUT", OSDP_CMD_OUTPUT);
	ADD_CONST("CMD_LED",    OSDP_CMD_LED);
	ADD_CONST("CMD_BUZZER", OSDP_CMD_BUZZER);
	ADD_CONST("CMD_TEXT",   OSDP_CMD_TEXT);
	ADD_CONST("CMD_COMSET", OSDP_CMD_COMSET);
	ADD_CONST("CMD_KEYSET", OSDP_CMD_KEYSET);

	/* enum osdp_led_color_e */
	ADD_CONST("LED_COLOR_NONE",  OSDP_LED_COLOR_NONE);
	ADD_CONST("LED_COLOR_RED",   OSDP_LED_COLOR_RED);
	ADD_CONST("LED_COLOR_GREEN", OSDP_LED_COLOR_GREEN);
	ADD_CONST("LED_COLOR_AMBER", OSDP_LED_COLOR_AMBER);
	ADD_CONST("LED_COLOR_BLUE",  OSDP_LED_COLOR_BLUE);

	/* enum osdp_card_formats_e */
	ADD_CONST("CARD_FMT_RAW_UNSPECIFIED", OSDP_CARD_FMT_RAW_UNSPECIFIED);
	ADD_CONST("CARD_FMT_RAW_WIEGAND",     OSDP_CARD_FMT_RAW_WIEGAND);
	ADD_CONST("CARD_FMT_ASCII",           OSDP_CARD_FMT_ASCII);

	/* enum osdp_pd_cap_function_code_e */
	ADD_CONST("CAP_UNUSED",                  OSDP_PD_CAP_UNUSED);
	ADD_CONST("CAP_CONTACT_STATUS_MONITORING", OSDP_PD_CAP_CONTACT_STATUS_MONITORING);
	ADD_CONST("CAP_OUTPUT_CONTROL",          OSDP_PD_CAP_OUTPUT_CONTROL);
	ADD_CONST("CAP_CARD_DATA_FORMAT",        OSDP_PD_CAP_CARD_DATA_FORMAT);
	ADD_CONST("CAP_READER_LED_CONTROL",      OSDP_PD_CAP_READER_LED_CONTROL);
	ADD_CONST("CAP_READER_AUDIBLE_OUTPUT",   OSDP_PD_CAP_READER_AUDIBLE_OUTPUT);
	ADD_CONST("CAP_READER_TEXT_OUTPUT",      OSDP_PD_CAP_READER_TEXT_OUTPUT);
	ADD_CONST("CAP_TIME_KEEPING",            OSDP_PD_CAP_TIME_KEEPING);
	ADD_CONST("CAP_CHECK_CHARACTER_SUPPORT", OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT);
	ADD_CONST("CAP_COMMUNICATION_SECURITY",  OSDP_PD_CAP_COMMUNICATION_SECURITY);
	ADD_CONST("CAP_RECEIVE_BUFFERSIZE",      OSDP_PD_CAP_RECEIVE_BUFFERSIZE);
	ADD_CONST("CAP_LARGEST_COMBINED_MESSAGE_SIZE", OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE);
	ADD_CONST("CAP_SMART_CARD_SUPPORT",      OSDP_PD_CAP_SMART_CARD_SUPPORT);
	ADD_CONST("CAP_READERS",                 OSDP_PD_CAP_READERS);
	ADD_CONST("CAP_BIOMETRICS",              OSDP_PD_CAP_BIOMETRICS);

#undef ADD_CONST
}

PyMODINIT_FUNC PyInit_osdp(void)
{
	PyObject *module;

	module = PyModule_Create(&osdp_module);

	pyosdp_add_module_constants(module);
	pyosdp_add_type_cp(module);
	pyosdp_add_type_pd(module);

	return module;
}
