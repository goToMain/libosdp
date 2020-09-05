#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import osdp
import random

pd_info = {
    "address": 101,
    "channel_type": "message_queue",
    "channel_speed": 115200,
    "channel_device": '/tmp/osdp_mq',

    "version": 1,
    "model": 1,
    "vendor_code": 0xCAFEBABE,
    "serial_number": 0xDEADBEAF,
    "firmware_version": 0x0000F00D
}

pd_cap = [
    {
        "function_code": osdp.CAP_OUTPUT_CONTROL,
        "compliance_level": 1,
        "num_items": 1
    },
]

def handle_command(address, command):
    if address != pd_info['address']:
        return -1 # error
    print("PD received command: ", command)
    return 0 # success

pd = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
pd.set_command_callback(handle_command)
pd.set_loglevel(7)

card_read_event = {
    "event": osdp.EVENT_CARDREAD,
    "reader_no": 1,
    "format": osdp.CARD_FMT_ASCII,
    "direction": 1,
    "data": bytes([ord('p'),ord('y'),ord('o'),ord('s'),ord('d'),ord('p')])
}

keypress_event = {
    "event": osdp.EVENT_KEYPRESS,
    "reader_no": 1,
    "data": bytes([3,1,3,3,7])
}

events = [ card_read_event, keypress_event ]

count = 0
while True:
    pd.refresh()

    if (count % 100 == 99):
        # send a random event to the CP
        r = random.randint(0, len(events)-1)
        pd.notify_event(events[r])

    time.sleep(0.05)
    count += 1
