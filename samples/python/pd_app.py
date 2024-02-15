#
#  Copyright (c) 2020-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
from osdp import *

## Describe the PD (setting scbk=None puts the PD in install mode)
pd_info = PDInfo(101, scbk=None, name='chn-0')

## Indicate the PD's capabilities to LibOSDP.
pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 2, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

## Create a PD device and kick-off the handler thread
pd = PeripheralDevice(pd_info, pd_cap, log_level=LogLevel.Debug)
pd.start()
pd.sc_wait()

## create a card read event to be used later
card_event = {
    'event': Event.CardRead,
    'reader_no': 1,
    'direction': 1,
    'format': CardFormat.ASCII,
    'data': bytes([9,1,9,2,6,3,1,7,7,0]),
}

count = 0 # loop counter

while True:
    ## Check if we have any commands from the CP
    cmd = pd.get_command(timeout=2)
    if cmd:
        print(f"PD: Received command: {cmd}")

    ## Send a card read event to CP
    pd.notify_event(card_event)

    if count >= 5:
        break
    count += 1

pd.stop()
