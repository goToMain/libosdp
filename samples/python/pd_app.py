#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import time
import random
import osdp

pd_cap = [
    {
        "function_code": osdp.CAP_OUTPUT_CONTROL,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": osdp.CAP_READER_LED_CONTROL,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": osdp.CAP_READER_AUDIBLE_OUTPUT,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": osdp.CAP_READER_TEXT_OUTPUT,
        "compliance_level": 1,
        "num_items": 1
    },
]

keystore_file = "/tmp/pd_scbk"

def store_scbk(key):
    with open(keystore_file, "w") as f:
        f.write(key.hex())

def load_scbk():
    key = bytes()
    if os.path.exists(keystore_file):
        with open(keystore_file, "r") as f:
            key = bytes.fromhex(f.read())
    return key

pd_info = {
    "address": 101,
    "flags": 0,
    "scbk": load_scbk(),
    "channel_type": "message_queue",
    "channel_speed": 115200,
    "channel_device": '/tmp/osdp_mq',

    # PD_ID
    "version": 1,
    "model": 1,
    "vendor_code": 0xCAFEBABE,
    "serial_number": 0xDEADBEAF,
    "firmware_version": 0x0000F00D
}

def handle_command(command):
    print("PD received command: ", command)

    if command['command'] == osdp.CMD_KEYSET:
        if command['type'] == 1:
            store_scbk(command['data'])

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

pd = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
pd.set_command_callback(handle_command)

# Print LibOSDP version and source info
print("pyosdp", "Version:", pd.get_version(),
                "Info:", pd.get_source_info())
pd.set_loglevel(osdp.LOG_DEBUG)


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

def main():
    global pd
    events = [ card_read_event, keypress_event ]

    count = 0 # loop counter

    while True:
        pd.refresh()

        if (count % 100 == 99) and pd.is_sc_active():
            # send a random event to the CP
            r = random.randint(0, len(events)-1)
            pd.notify_event(events[r])

        if count == 50:
            pd = None
            print("pd Set to None")
            time.sleep(5)
            return 0

        count += 1
        time.sleep(0.020) #sleep for 20ms

if __name__ == "__main__":
    main()
