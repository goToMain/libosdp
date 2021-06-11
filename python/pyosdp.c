/*
 * Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

#define pyosdp_set_loglevel_doc \
	"Set OSDP logging level\n" \
	"\n" \
	"@param log_level OSDP log level (0 to 7)\n" \
	"\n" \
	"@return None"
static PyObject *pyosdp_set_loglevel(pyosdp_t *self, PyObject *args)
{
	int log_level;

	if (!PyArg_ParseTuple(args, "I", &log_level))
		return NULL;

	if (log_level <= 0 || log_level > 7) {
		PyErr_SetString(PyExc_KeyError, "invalid log level");
		return NULL;
	}

	osdp_logger_init(log_level, printf);

	Py_RETURN_NONE;
}

PyObject *pyosdp_get_version(pyosdp_t *self, PyObject *args)
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

PyObject *pyosdp_get_source_info(pyosdp_t *self, PyObject *args)
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

static PyMethodDef osdp_funcs[] = {
	{
		"get_version",
		(PyCFunction)pyosdp_get_version,
		METH_NOARGS,
		"Get OSDP version as string"
	},
	{
		"get_source_info",
		(PyCFunction)pyosdp_get_source_info,
		METH_NOARGS,
		"Get LibOSDP source info string"
	},
	{
		"set_loglevel",
		(PyCFunction)pyosdp_set_loglevel,
		METH_VARARGS,
		pyosdp_set_loglevel_doc
	},
	{ NULL, NULL, 0, NULL } /* sentinel */
};

static PyModuleDef osdp_module = {
	PyModuleDef_HEAD_INIT,
	.m_name    = "osdp",
	.m_doc     = "Open Supervised Device Protocol",
	.m_size    = -1,
	.m_methods = osdp_funcs,
};

void pyosdp_add_module_constants(PyObject *module)
{
#define ADD_CONST(k,v) PyModule_AddIntConstant(module, k, v);

	/* setup flags */
	ADD_CONST("FLAG_ENFORCE_SECURE", OSDP_FLAG_ENFORCE_SECURE);

	/* enum osdp_cmd_e */
	ADD_CONST("CMD_OUTPUT", OSDP_CMD_OUTPUT);
	ADD_CONST("CMD_LED",    OSDP_CMD_LED);
	ADD_CONST("CMD_BUZZER", OSDP_CMD_BUZZER);
	ADD_CONST("CMD_TEXT",   OSDP_CMD_TEXT);
	ADD_CONST("CMD_COMSET", OSDP_CMD_COMSET);
	ADD_CONST("CMD_KEYSET", OSDP_CMD_KEYSET);
	ADD_CONST("CMD_MFG",    OSDP_CMD_MFG);

	/* enum osdp_event_type */
	ADD_CONST("EVENT_CARDREAD", OSDP_EVENT_CARDREAD);
	ADD_CONST("EVENT_KEYPRESS", OSDP_EVENT_KEYPRESS);
	ADD_CONST("EVENT_MFGREP",   OSDP_EVENT_MFGREP);

	/* enum osdp_led_color_e */
	ADD_CONST("LED_COLOR_NONE",  OSDP_LED_COLOR_NONE);
	ADD_CONST("LED_COLOR_RED",   OSDP_LED_COLOR_RED);
	ADD_CONST("LED_COLOR_GREEN", OSDP_LED_COLOR_GREEN);
	ADD_CONST("LED_COLOR_AMBER", OSDP_LED_COLOR_AMBER);
	ADD_CONST("LED_COLOR_BLUE",  OSDP_LED_COLOR_BLUE);

	/* enum osdp_event_cardread_format_e */
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
