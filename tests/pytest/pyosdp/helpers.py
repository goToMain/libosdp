import time
#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

def osdp_refresh(event, lock, ctx):
    while not event.is_set():
        lock.acquire()
        ctx.refresh()
        lock.release()
        time.sleep(0.020) #sleep for 20ms