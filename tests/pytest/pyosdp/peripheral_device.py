#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp
import time
import queue
import threading

from .helpers import osdp_refresh

class PeripheralDevice():
    def __init__(self, pd_info, pd_cap):
        osdp.set_loglevel(7)
        self.command_queue = queue.Queue()
        self.pd_ctx = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
        self.pd_ctx.set_command_callback(self.command_handler)
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.pd_ctx,)
        self.thread = threading.Thread(name='pd', target=osdp_refresh, args=args)

    def command_handler(self, command):
        self.command_queue.put(command)
        return { "return_code": 0 }

    def get_command(self, timeout=0):
        try:
            cmd = self.command_queue.get(timeout=timeout)
        except queue.Empty:
            return None
        return cmd

    def start(self):
        if self.thread:
            self.thread.start()
            return True
        return False

    def stop(self):
        while self.thread and self.thread.is_alive():
            self.event.set()
            self.thread.join(2)
            if not self.thread.is_alive():
                return True
        return False
