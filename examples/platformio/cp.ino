/*
 * Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <osdp.hpp>

OSDP::ControlPanel cp;

int serial1_send_func(void *data, uint8_t *buf, int len)
{
    (void)(data);

    int sent = 0;
    for (int i = 0; i < len; i++) {
        if (Serial1.write(buf[i])) {
            sent++;
        } else {
            break;
        }
    }
    return sent;
}

int serial1_recv_func(void *data, uint8_t *buf, int len)
{
    (void)(data);

    int read = 0;
    while (Serial1.available() && read < len) {
        buf[read] = Serial1.read();
        read++;
    }
    return read;
}

osdp_pd_info_t pd_info[] = {
    {
        .name = "pd[101]",
        .baud_rate = (int)115200,
        .address = 101,
        .flags = 0,
        .id = {},
        .cap = nullptr,
        .channel = {
            .data = nullptr,
            .id = 0,
            .recv = serial1_recv_func,
            .send = serial1_send_func,
            .flush = nullptr
        },
        .scbk = nullptr,
    },
};

int event_handler(void *data, int pd, struct osdp_event *event)
{
    (void)(data);

    Serial.println("Received an event!");
    return 0;
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);

    cp.logger_init("osdp::cp", OSDP_LOG_DEBUG, NULL);

    cp.setup(1, pd_info);
    cp.set_event_callback(event_handler, nullptr);
}

void loop()
{
    cp.refresh();
    delay(1000);
}
