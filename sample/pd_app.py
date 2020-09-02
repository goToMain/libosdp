#
#  Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import osdp

pd_info = {
    "address": 101,
    "channel_type": "message_queue",
    "channel_speed": 115200,
    "channel_device": '/tmp/a.p',

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

pd = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
pd.set_loglevel(7)

count = 0
while True:
    pd.refresh()

    time.sleep(0.05)
    count += 1
