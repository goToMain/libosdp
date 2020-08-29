/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <utils/serial.h>

static int channel_uart_send(void *data, uint8_t *buf, int len)
{
	return serial_write(data, (unsigned char *)buf, len);
}

static int channel_uart_recv(void *data, uint8_t *buf, int maxLen)
{
	return serial_read(data, (unsigned char *)buf, maxLen);
}

static void channel_uart_flush(void *data)
{
	serial_flush(data);
}

void pyosdp_close_channel(struct osdp_channel *channel)
{
	serial_close(channel->data);
}

int pyosdp_open_channel(struct osdp_channel *channel, const char *device,
			int baud_rate)
{
	struct serial *ctx;

	ctx = serial_open(device, baud_rate, "8N1");
	if (ctx == NULL) {
		PyErr_SetString(PyExc_PermissionError, "failed to open device");
		return -1;
	}
	channel->data = (void *)ctx;
	channel->send = channel_uart_send;
	channel->recv = channel_uart_recv;
	channel->flush = channel_uart_flush;
	return 0;
}

int pyosdp_build_pd_info(osdp_pd_info_t *info, int address, int baud_rate)
{
	info->address = address;
	info->baud_rate = baud_rate;
	info->flags = 0;
	info->cap = NULL;
	return 0;
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
