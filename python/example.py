#
#  Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import threading
import osdp

def keypress_event_handler(address, key):
    print("Got keypress event. address: ", address, " Key: ", key)

def cardread_event_handler(address, fmt, card_data):
    print("Got cardread event. address: ", address, " format: ", fmt,
          "card data: ", card_data)

callbacks = {
    "keypress": keypress_event_handler,
    "cardread": cardread_event_handler
}

key = '01020304050607080910111213141516'

pd_info = [
    (1, 'msgq', 115200, '/Users/csiddharth/work/osdp/libosdp/build/msgq-pd-0.cfg')
]

cp = osdp.ControlPanel(pd_info, master_key=key)
cp.set_callback(**callbacks)

output_cmd = {
    "command": osdp.CMD_OUTPUT,
    "number": 1,
    "control_code": 1,
    "timer": 10
}

buzzer_cmd = {
    "command": osdp.CMD_BUZZER,
    "control_code": 1,
    "on_count": 10,
    "off_count": 10,
    "rep_count": 10
}

text_cmd = {
    "command": osdp.CMD_TEXT,
    "control_code": 1,
    "timer": 20,
    "row": 1,
    "col": 1,
    "data": "PYOSDP"
}

led_cmd = {
    "command": osdp.CMD_LED,
    "control_code": 1,
    "on_count": 10,
    "off_count": 10,
    "on_color": osdp.LED_COLOR_RED,
    "off_color": osdp.LED_COLOR_NONE,
    "timer": 10,
    "reader": 1,
    "number": 1,
    "permanent": True
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

count = 0
while True:
    cp.refresh()
    if (count % 100 == 99):
        cp.send(pd=0, **keyset_cmd)
    time.sleep(0.05)
    count += 1
