/*
 * Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <osdp.hpp>

OSDP::PeripheralDevice pd;

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

osdp_pd_info_t info_pd = {
    .name = "pd[101]",
    .baud_rate = 9600,
    .address = 101,
    .flags = 0,
    .id = {
        .version = 1,
        .model = 153,
        .vendor_code = 31337,
        .serial_number = 0x01020304,
        .firmware_version = 0x0A0B0C0D,
    },
    .cap = (struct osdp_pd_cap []) {
        {
            .function_code = OSDP_PD_CAP_READER_LED_CONTROL,
            .compliance_level = 1,
            .num_items = 1
        },
        {
            .function_code = OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,
            .compliance_level = 1,
            .num_items = 1
        },
        { static_cast<uint8_t>(-1), 0, 0 } /* Sentinel */
    },
    .channel = {
        .data = nullptr,
        .id = 0,
        .recv = serial1_recv_func,
        .send = serial1_send_func,
        .flush = nullptr
    },
    .scbk = nullptr,
};

int pd_command_handler(void *data, struct osdp_cmd *cmd)
{
    (void)(data);

    Serial.println("Received a command!");
    return 0;
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);

    pd.logger_init("osdp::pd", OSDP_LOG_DEBUG, NULL);

    pd.setup(&info_pd);

    pd.set_command_callback(pd_command_handler, nullptr);
}

void loop()
{
    pd.refresh();
    delay(1000);
}