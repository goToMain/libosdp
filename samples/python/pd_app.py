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
        return { "return_code": -1 }

    print("PD received command: ", command)

    if command['command'] == osdp.CMD_MFG:
        # For MFG commands, repply wth MFGREP. Notice that the "command"
        # is still set be osdp.CMD_MFG. This is because the structure for both
        # MFG and MFGREP are the same.
        return {
            "return_code": 1,
            "command": osdp.CMD_MFG,
            "vendor_code": 0x00030201,
            "mfg_command": 14,
            "data": bytes([1,2,3,4,5,6,7,8])
        }

    return { "return_code": 0 }

# Print LibOSDP version and source info
print("pyosdp", "Version:", osdp.get_version(),
                "Info:", osdp.get_source_info())

pd = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
pd.set_command_callback(handle_command)
pd.set_loglevel(6)

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

count = 0 # loop counter

while True:
    pd.refresh()

    if (count % 100 == 99) and pd.sc_active():
        # send a random event to the CP
        r = random.randint(0, len(events)-1)
        pd.notify_event(events[r])

    count += 1
    time.sleep(0.020) #sleep for 20ms
