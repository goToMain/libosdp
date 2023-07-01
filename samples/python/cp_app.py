#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import random
import osdp

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
    "led_number": 0,
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
    "address": 101,
    "baud_rate": 9600
}

keyset_cmd = {
    "command": osdp.CMD_KEYSET,
    "type": 1,
    "data": bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
}

mfg_cmd = {
    "command": osdp.CMD_MFG,
    "vendor_code": 0x00030201,
    "mfg_command": 13,
    "data": bytes([9,1,9,2,6,3,1,7,7,0])
}

key = bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])

pd_info = [
    # PD_0 info
    {
        "address": 101,
        "flags": 0, # osdp.FLAG_ENFORCE_SECURE
        "scbk": key,
        "channel_type": "message_queue",
        "channel_speed": 115200,
        "channel_device": '/tmp/osdp_mq',
    }
]

commands = [ output_cmd, buzzer_cmd, text_cmd, led_cmd, comset_cmd, mfg_cmd, keyset_cmd ]

def event_handler(address, event):
    print("Address: ", address, " Event: ", event)

cp = osdp.ControlPanel(pd_info)

# Print LibOSDP version and source info
print("pyosdp", "Version:", cp.get_version(),
                "Info:", cp.get_source_info())
cp.set_loglevel(osdp.LOG_DEBUG)

cp.set_event_callback(event_handler)

def main():
    global cp
    count = 0  # loop counter
    PD_0 = 0   # PD offset number

    while True:
        cp.refresh()

        if (count % 100) == 99 and cp.is_sc_active(PD_0):
            # send a random command to the PD_0
            r = random.randint(PD_0, len(commands)-1)
            cp.send_command(PD_0, commands[r])

        count += 1
        time.sleep(0.020) #sleep for 20ms

if __name__ == "__main__":
    main()
