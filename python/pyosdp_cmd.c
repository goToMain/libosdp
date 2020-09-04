/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

int pyosdp_cmd_make_dict(PyObject **dict, struct osdp_cmd *cmd)
{
	char buf[64];
	int is_temporary = 0;
	PyObject *obj, *tmp;
	struct osdp_cmd_led_params *p = &cmd->led.permanent;

	obj = PyDict_New();
	if (obj == NULL)
		return -1;

	if (pyosdp_dict_add_int(obj, "command", cmd->id))
		return -1;

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->output.control_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "output_no", cmd->output.output_no))
			return -1;
		if (pyosdp_dict_add_int(obj, "timer_count", cmd->output.timer_count))
			return -1;
		break;
	case OSDP_CMD_LED:
		if (cmd->led.temporary.control_code != 0) {
			p = &cmd->led.temporary;
			is_temporary = 1;
		}
		if (is_temporary) {
			if (pyosdp_dict_add_int(obj, "temporary", 1))
				return -1;
		}
		if (pyosdp_dict_add_int(obj, "led_number", cmd->led.led_number))
			return -1;
		if (pyosdp_dict_add_int(obj, "reader", cmd->led.reader))
			return -1;
		if (pyosdp_dict_add_int(obj, "control_code", p->control_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "off_color", p->off_color))
			return -1;
		if (pyosdp_dict_add_int(obj, "on_color", p->on_color))
			return -1;
		if (pyosdp_dict_add_int(obj, "on_count", p->on_count))
			return -1;
		if (pyosdp_dict_add_int(obj, "off_count", p->off_count))
			return -1;
		if (pyosdp_dict_add_int(obj, "timer_count", p->timer_count))
			return -1;
		break;
	case OSDP_CMD_BUZZER:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->buzzer.control_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "on_count", cmd->buzzer.on_count))
			return -1;
		if (pyosdp_dict_add_int(obj, "off_count", cmd->buzzer.off_count))
			return -1;
		if (pyosdp_dict_add_int(obj, "reader", cmd->buzzer.reader))
			return -1;
		if (pyosdp_dict_add_int(obj, "rep_count", cmd->buzzer.rep_count))
			return -1;
		break;
	case OSDP_CMD_TEXT:
		if (pyosdp_dict_add_int(obj, "control_code", cmd->text.control_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "offset_col", cmd->text.offset_col))
			return -1;
		if (pyosdp_dict_add_int(obj, "offset_row", cmd->text.offset_row))
			return -1;
		if (pyosdp_dict_add_int(obj, "reader", cmd->text.reader))
			return -1;
		if (pyosdp_dict_add_int(obj, "reader", cmd->text.reader))
			return -1;
		if (cmd->text.length > (sizeof(buf)-1))
			return -1;
		memcpy(buf, cmd->text.data, cmd->text.length);
		buf[cmd->text.length] = '\0';
		if (pyosdp_dict_add_str(obj, "data", buf))
			return -1;
		break;
	case OSDP_CMD_KEYSET:
		if (pyosdp_dict_add_int(obj, "type", cmd->keyset.type))
			return -1;
		if (cmd->keyset.length > 16)
			return -1;
		if (atohstr(buf, cmd->keyset.data, cmd->keyset.length))
			return -1;
		if (pyosdp_dict_add_str(obj, "data", buf))
			return -1;
		break;
	case OSDP_CMD_COMSET:
		if (pyosdp_dict_add_int(obj, "address", cmd->comset.address))
			return -1;
		if (pyosdp_dict_add_int(obj, "baud_rate", cmd->comset.baud_rate))
			return -1;
		break;
	case OSDP_CMD_MFG:
		if (pyosdp_dict_add_int(obj, "vendor_code", cmd->mfg.vendor_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "mfg_command", cmd->mfg.command))
			return -1;
		tmp = Py_BuildValue("y#", cmd->mfg.data, cmd->mfg.length);
		if (tmp == NULL)
			return -1;
		if (PyDict_SetItemString(obj, "data", tmp))
			return -1;
		Py_DECREF(tmp);
		break;
	default:
		PyErr_SetString(PyExc_NotImplementedError,
				"command not implemented");
		return -1;
	}

	*dict = obj;
	return 0;
}


static int pyosdp_handle_cmd_output(struct osdp_cmd *p, PyObject *dict)
{
	struct osdp_cmd_output *cmd = &p->output;
	int output_no, control_code, timer_count;

	p->id = OSDP_CMD_OUTPUT;

	if (pyosdp_dict_get_int(dict, "output_no", &output_no))
		return -1;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		return -1;

	if (pyosdp_dict_get_int(dict, "timer_count", &timer_count))
		return -1;

	cmd->output_no = (uint8_t)output_no;
	cmd->control_code = (uint8_t)control_code;
	cmd->timer_count = (uint8_t)timer_count;
	return 0;
}

static int pyosdp_handle_cmd_led(struct osdp_cmd *p, PyObject *dict)
{
	int led_number, reader, off_color, on_color,
	    off_count, on_count, timer_count, control_code;
	struct osdp_cmd_led *cmd = &p->led;
	struct osdp_cmd_led_params *tmp = &cmd->permanent;

	p->id = OSDP_CMD_LED;

	if (pyosdp_dict_get_int(dict, "led_number", &led_number))
		return -1;

	if (pyosdp_dict_get_int(dict, "reader", &reader))
		return -1;

	if (PyDict_GetItemString(dict, "temporary"))
		tmp = &cmd->temporary;

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

	cmd->led_number = (uint8_t)led_number;
	cmd->reader = (uint8_t)reader;
	tmp->control_code = (uint8_t)control_code;
	tmp->off_color = (uint8_t)off_color;
	tmp->on_color = (uint8_t)on_color;
	tmp->on_count = (uint8_t)on_count;
	tmp->off_count = (uint8_t)off_count;
	tmp->timer_count = (uint8_t)timer_count;
	return 0;
}

static int pyosdp_handle_cmd_buzzer(struct osdp_cmd *p, PyObject *dict)
{
	struct osdp_cmd_buzzer *cmd = &p->buzzer;
	int reader, on_count, off_count, rep_count, control_code;

	p->id = OSDP_CMD_BUZZER;

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

	cmd->reader = (uint8_t)reader;
	cmd->on_count = (uint8_t)on_count;
	cmd->off_count = (uint8_t)off_count;
	cmd->rep_count = (uint8_t)rep_count;
	cmd->control_code = (uint8_t)control_code;
	return 0;
}

static int pyosdp_handle_cmd_text(struct osdp_cmd *p, PyObject *dict)
{
	int ret = -1;
	char *data = NULL;
	struct osdp_cmd_text *cmd = &p->text;
	int length, reader, control_code, offset_row, offset_col, temp_time;

	p->id = OSDP_CMD_TEXT;

	if (pyosdp_dict_get_str(dict, "data", &data))
		goto exit;

	length = strlen(data);
	if (length > OSDP_CMD_TEXT_MAX_LEN)
		goto exit;

	if (pyosdp_dict_get_int(dict, "reader", &reader))
		goto exit;

	if (pyosdp_dict_get_int(dict, "control_code", &control_code))
		goto exit;

	if (pyosdp_dict_get_int(dict, "offset_col", &offset_col))
		goto exit;

	if (pyosdp_dict_get_int(dict, "offset_row", &offset_row))
		goto exit;

	if (pyosdp_dict_get_int(dict, "temp_time", &temp_time))
		goto exit;

	cmd->reader = (uint8_t)reader;
	cmd->control_code = (uint8_t)control_code;
	cmd->length = (uint8_t)length;
	cmd->offset_col = (uint8_t)offset_col;
	cmd->offset_row = (uint8_t)offset_row;
	cmd->temp_time = (uint8_t)temp_time;
	memcpy(cmd->data, data, length);
	ret = 0;
exit:
	safe_free(data);
	return ret;
}

static int pyosdp_handle_cmd_keyset(struct osdp_cmd *p, PyObject *dict)
{
	int ret = -1, type;
	struct osdp_cmd_keyset *cmd = &p->keyset;
	char *data = NULL;

	p->id = OSDP_CMD_KEYSET;

	if (pyosdp_dict_get_int(dict, "type", &type))
		goto exit;

	if (pyosdp_dict_get_str(dict, "data", &data))
		goto exit;

	cmd->type = (uint8_t)type;
	cmd->length = strlen(data);
	if (cmd->length > (OSDP_CMD_KEYSET_KEY_MAX_LEN * 2))
		goto exit;
	cmd->length = hstrtoa(cmd->data, data);
	if (cmd->length < 0)
		goto exit;
	ret = 0;
exit:
	safe_free(data);
	return ret;
}

static int pyosdp_handle_cmd_comset(struct osdp_cmd *p, PyObject *dict)
{
	struct osdp_cmd_comset *cmd = &p->comset;
	int address, baud_rate;

	p->id = OSDP_CMD_COMSET;

	if (pyosdp_dict_get_int(dict, "address", &address))
		return -1;

	if (pyosdp_dict_get_int(dict, "baud_rate", &baud_rate))
		return -1;

	cmd->address = (uint8_t)address;
	cmd->baud_rate = (uint32_t)baud_rate;
	return 0;
}

static int pyosdp_handle_cmd_mfg(struct osdp_cmd *p, PyObject *dict)
{
	int i, data_length;
	struct osdp_cmd_mfg *cmd = &p->mfg;
	uint8_t *data_bytes;
	int vendor_code, mfg_command;
	PyObject *data;

	p->id = OSDP_CMD_MFG;

	if (pyosdp_dict_get_int(dict, "vendor_code", &vendor_code))
		return -1;

	if (pyosdp_dict_get_int(dict, "mfg_command", &mfg_command))
		return -1;

	data = PyDict_GetItemString(dict, "data");
	if (data == NULL) {
		PyErr_Format(PyExc_KeyError, "Key 'data' not found");
		return -1;
	}

	if (!PyArg_Parse(data, "y#", &data_bytes, &data_length))
		return -1;

	if (data_bytes == NULL || data_length == 0) {
		PyErr_Format(PyExc_ValueError, "Unable to extact data bytes");
		return -1;
	}

	cmd->vendor_code = (uint32_t)vendor_code;
	cmd->command = mfg_command;
	cmd->length = data_length;
	for (i = 0; i < cmd->length; i++)
		cmd->data[i] = data_bytes[i];
	return 0;
}

int pyosdp_cmd_make_struct(struct osdp_cmd *cmd, PyObject *dict)
{
	int cmd_id;

	if (pyosdp_dict_get_int(dict, "command", &cmd_id))
		return -1;

	switch(cmd_id) {
	case OSDP_CMD_OUTPUT:
		return pyosdp_handle_cmd_output(cmd, dict);
	case OSDP_CMD_LED:
		return pyosdp_handle_cmd_led(cmd, dict);
	case OSDP_CMD_BUZZER:
		return pyosdp_handle_cmd_buzzer(cmd, dict);
	case OSDP_CMD_TEXT:
		return pyosdp_handle_cmd_text(cmd, dict);
	case OSDP_CMD_KEYSET:
		return pyosdp_handle_cmd_keyset(cmd, dict);
	case OSDP_CMD_COMSET:
		return pyosdp_handle_cmd_comset(cmd, dict);
	case OSDP_CMD_MFG:
		return pyosdp_handle_cmd_mfg(cmd, dict);
	default:
		PyErr_SetString(PyExc_NotImplementedError,
				"command not implemented");
		return -1;
	}
	return 0;
}
