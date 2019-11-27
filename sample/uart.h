/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>

int uart_write(uint8_t *buf, int len);
int uart_read(uint8_t *buf, int maxLen);
int uart_init(const char *dev, int baud_rate);

#endif  /* _UART_H_ */
