#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp
import time
import queue
import threading

from .helpers import PDInfo, PDCapabilities
from .constants import LogLevel

class PeripheralDevice():
    def __init__(self, pd_info: PDInfo, pd_cap: PDCapabilities,
                 log_level: LogLevel=LogLevel.Info):
        self.command_queue = queue.Queue()
        self.ctx = osdp.PeripheralDevice(pd_info.get(), capabilities=pd_cap.get())
        self.ctx.set_loglevel(log_level)
        self.ctx.set_command_callback(self.command_handler)
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.ctx,)
        self.thread = threading.Thread(name='pd', target=self.refresh, args=args)

    @staticmethod
    def refresh(event, lock, ctx):
        while not event.is_set():
            lock.acquire()
            ctx.refresh()
            lock.release()
            time.sleep(0.020) #sleep for 20ms

    def command_handler(self, command):
        self.command_queue.put(command)
        return { "return_code": 0 }

    def get_command(self, timeout: int=5):
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
