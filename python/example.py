#
#  Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import threading
import osdp

key = '01020304050607080910111213141516'

pd_info = [
    (101, 'uart', 115200, '/dev/tty.usbserial-AHYJSSFN')
]

cp = osdp.ControlPanel(pd_info, master_key=key)

def osdp_refresh_thread():
    while True:
        cp.refresh()
        time.sleep(0.050)

t = threading.Thread(target=osdp_refresh_thread)
t.daemon = True
t.start()

input("Press any to exit...\n")
