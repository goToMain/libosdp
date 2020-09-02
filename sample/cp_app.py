#
#  Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import random
import osdp

def keypress_event_handler(address, key):
    print("Got keypress event. address: ", address, " Key: ", key)

def cardread_event_handler(address, fmt, card_data):
    print("Got cardread event. address: ", address, " format: ", fmt,
          "card data: ", card_data)

output_cmd = {
    "command": osdp.CMD_OUTPUT,
    "output_no": 0,
    "control_code": 1,
    "timer_count": 10
}

buzzer_cmd = {
    "command": osdp.CMD_BUZZER,
    "reader": 0,
    "control_code": 1,
    "on_count": 10,
    "off_count": 10,
    "rep_count": 10
}

text_cmd = {
    "command": osdp.CMD_TEXT,
    "reader": 0,
    "control_code": 1,
    "temp_time": 20,
    "offset_row": 1,
    "offset_col": 1,
    "data": "PYOSDP"
}

led_cmd = {
    "command": osdp.CMD_LED,
    "reader": 1,
    "led_number": 1,
    "control_code": 1,
    "on_count": 10,
    "off_count": 10,
    "on_color": osdp.LED_COLOR_RED,
    "off_color": osdp.LED_COLOR_NONE,
    "timer_count": 10,
    "temporary": True
}

comset_cmd = {
    "command": osdp.CMD_COMSET,
    "address": 1,
    "baud_rate": 9600
}

keyset_cmd = {
    "command": osdp.CMD_KEYSET,
    "type": 1,
    "data": "01020304050607080910111213141517"
}

pd_info = [
    {
        "address": 101,
        "channel_type": "message_queue",
        "channel_speed": 115200,
        "channel_device": '/tmp/a.p',
    }
]

callbacks = {
    "keypress": keypress_event_handler,
    "cardread": cardread_event_handler
}
key = '01020304050607080910111213141516'
commands = [ output_cmd, buzzer_cmd, text_cmd, led_cmd, comset_cmd ]

cp = osdp.ControlPanel(pd_info, master_key=key)
cp.set_callback(callbacks)
cp.set_loglevel(6)

count = 0
while True:
    cp.refresh()

    if (count % 100 == 99):
        # send a random command to the PD_0
        r = random.randint(0, len(commands)-1)
        cp.send(0, commands[r])

    time.sleep(0.05)
    count += 1
