/*
 * Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pyosdp.h"

int pyosdp_cmd_make_dict(PyObject **dict, struct osdp_cmd *cmd)
{
	char buf[64];
	int is_temporary = 0;
	PyObject *obj;
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
		if (pyosdp_dict_add_bytes(obj, "data", cmd->keyset.data, cmd->keyset.length))
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
		if (pyosdp_dict_add_bytes(obj, "data", cmd->mfg.data, cmd->mfg.length))
			return -1;
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
	int type, len;
	struct osdp_cmd_keyset *cmd = &p->keyset;
	uint8_t *buf;

	p->id = OSDP_CMD_KEYSET;

	if (pyosdp_dict_get_int(dict, "type", &type))
		return -1;

	if (pyosdp_dict_get_bytes(dict, "data", &buf, &len))
		return -1;

	cmd->type = (uint8_t)type;
	if (len > OSDP_CMD_KEYSET_KEY_MAX_LEN)
		return -1;
	cmd->length = len;
	memcpy(cmd->data, buf, len);
	return 0;
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

	p->id = OSDP_CMD_MFG;

	if (pyosdp_dict_get_int(dict, "vendor_code", &vendor_code))
		return -1;

	if (pyosdp_dict_get_int(dict, "mfg_command", &mfg_command))
		return -1;

	if (pyosdp_dict_get_bytes(dict, "data", &data_bytes, &data_length))
		return -1;

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

int pyosdp_make_event_dict(PyObject **dict, struct osdp_event *event)
{
	int tmp;
	PyObject *obj;

	obj = PyDict_New();
	if (obj == NULL)
		return -1;

	if (pyosdp_dict_add_int(obj, "event", event->type))
		return -1;

	switch (event->type) {
	case OSDP_EVENT_CARDREAD:
		if (pyosdp_dict_add_int(obj, "reader_no", event->cardread.reader_no))
			return -1;
		if (pyosdp_dict_add_int(obj, "format", event->cardread.format))
			return -1;
		if (pyosdp_dict_add_int(obj, "direction", event->cardread.direction))
			return -1;
		if (pyosdp_dict_add_int(obj, "length", event->cardread.length))
			return -1;
		if (event->cardread.format == OSDP_CARD_FMT_RAW_UNSPECIFIED ||
		    event->cardread.format == OSDP_CARD_FMT_RAW_WIEGAND)
			tmp = (event->cardread.length + 7) / 8; // bits to bytes
		else
			tmp = event->cardread.length;
		if (pyosdp_dict_add_bytes(obj, "data", event->cardread.data, tmp))
			return -1;
		break;
	case OSDP_EVENT_KEYPRESS:
		if (pyosdp_dict_add_int(obj, "reader_no", event->keypress.reader_no))
			return -1;
		if (pyosdp_dict_add_int(obj, "length", event->keypress.length))
			return -1;
		if (pyosdp_dict_add_bytes(obj, "data", event->keypress.data, event->keypress.length))
			return -1;
		break;
	case OSDP_EVENT_MFGREP:
		if (pyosdp_dict_add_int(obj, "vendor_code", event->mfgrep.vendor_code))
			return -1;
		if (pyosdp_dict_add_int(obj, "mfg_command", event->mfgrep.command))
			return -1;
		if (pyosdp_dict_add_bytes(obj, "data", event->mfgrep.data, event->mfgrep.length))
			return -1;
		break;
	default:
		PyErr_SetString(PyExc_NotImplementedError,
				"event cannot be handled");
		return -1;
	}
	*dict = obj;
	return 0;
}

static int pyosdp_make_cardread_event_struct(struct osdp_event *p, PyObject *dict)
{
	int i, data_length, reader_no, format, direction, len_bytes;
	struct osdp_event_cardread *ev = &p->cardread;
	uint8_t *data_bytes;

	p->type = OSDP_EVENT_CARDREAD;

	if (pyosdp_dict_get_int(dict, "reader_no", &reader_no))
		return -1;

	if (pyosdp_dict_get_int(dict, "format", &format))
		return -1;

	if (pyosdp_dict_get_int(dict, "direction", &direction))
		return -1;

	if (pyosdp_dict_get_bytes(dict, "data", &data_bytes, &data_length))
		return -1;

	if (format == OSDP_CARD_FMT_RAW_WIEGAND ||
	    format == OSDP_CARD_FMT_RAW_UNSPECIFIED) {
		if (pyosdp_dict_get_int(dict, "length", &data_length))
			return -1;
		len_bytes = (data_length + 7) / 8; // bits to bytes
	} else {
		len_bytes = data_length;
	}

	if (len_bytes > OSDP_EVENT_MAX_DATALEN) {
		PyErr_Format(PyExc_ValueError, "Data bytes too long");
		return -1;
	}

	ev->reader_no = (uint8_t)reader_no;
	ev->format = (uint8_t)format;
	ev->direction = (uint8_t)direction;
	ev->length = data_length;
	for (i = 0; i < len_bytes; i++)
		ev->data[i] = data_bytes[i];
	return 0;
}

static int pyosdp_make_keypress_event_struct(struct osdp_event *p, PyObject *dict)
{
	int i, data_length, reader_no;
	struct osdp_event_keypress *ev = &p->keypress;
	uint8_t *data_bytes;

	p->type = OSDP_EVENT_KEYPRESS;

	if (pyosdp_dict_get_int(dict, "reader_no", &reader_no))
		return -1;

	if (pyosdp_dict_get_bytes(dict, "data", &data_bytes, &data_length))
		return -1;

	ev->reader_no = (uint8_t)reader_no;
	ev->length = data_length;
	for (i = 0; i < ev->length; i++)
		ev->data[i] = data_bytes[i];
	return 0;
}

int pyosdp_make_event_struct(struct osdp_event *event, PyObject *dict)
{
	int event_type;

	if (pyosdp_dict_get_int(dict, "event", &event_type))
		return -1;

	switch (event_type) {
	case OSDP_EVENT_CARDREAD:
		if (pyosdp_make_cardread_event_struct(event, dict))
			return -1;
		break;
	case OSDP_EVENT_KEYPRESS:
		if (pyosdp_make_keypress_event_struct(event, dict))
			return -1;
		break;
	default:
		PyErr_Format(PyExc_ValueError, "Unknown event");
		return -1;
	}
	return 0;
}
