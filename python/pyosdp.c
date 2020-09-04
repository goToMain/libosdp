/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

static PyMethodDef osdp_funcs[] = {
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

	/* enum osdp_cmd_e */
	ADD_CONST("CMD_OUTPUT", OSDP_CMD_OUTPUT);
	ADD_CONST("CMD_LED",    OSDP_CMD_LED);
	ADD_CONST("CMD_BUZZER", OSDP_CMD_BUZZER);
	ADD_CONST("CMD_TEXT",   OSDP_CMD_TEXT);
	ADD_CONST("CMD_COMSET", OSDP_CMD_COMSET);
	ADD_CONST("CMD_KEYSET", OSDP_CMD_KEYSET);
	ADD_CONST("CMD_MFG",    OSDP_CMD_MFG);

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
