/*
 * Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

int64_t osdp_millis_now()
{
  return (int64_t)millis();
}
