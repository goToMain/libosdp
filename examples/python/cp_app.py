#
#  Copyright (c) 2020-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import serial
from osdp import *

class SerialChannel(Channel):
    def __init__(self, device: str, speed: int):
        self.dev = serial.Serial(device, speed, timeout=1)

    def read(self, max: int):
        return self.dev.read(max)

    def write(self, data: bytes):
        return self.dev.write(data)

    def flush(self):
        self.dev.flush()

    def __del__(self):
        self.dev.close()

## Describe the PD (setting scbk=None puts the PD in install mode)
channel = SerialChannel("/dev/ttyUSB0")
pd_info = [
    PDInfo(101, channel, scbk=KeyStore.gen_key()),
]

## Create a CP device and kick-off the handler thread
cp = ControlPanel(pd_info, log_level=LogLevel.Debug)
cp.start()
cp.sc_wait_all()

## create a LED Command to be used later
led_cmd = {
    'command': Command.LED,
    'reader': 1,
    'led_number': 0,
    'control_code': 1,
    'on_count': 10,
    'off_count': 10,
    'on_color': CommandLEDColor.Red,
    'off_color': CommandLEDColor.Black,
    'timer_count': 10,
    'temporary': True
}

count = 0  # loop counter
while True:
    ## Send LED command to PD-0
    cp.send_command(pd_info[0].address, led_cmd)

    ## Check if we have an event from PD
    event = cp.get_event(pd_info[0].address, timeout=2)
    if event:
        print(f"PD-0 Sent Event {event}")

    if count >= 5:
        break
    count += 1

cp.teardown()
