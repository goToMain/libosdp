#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
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
        self.address = pd_info.address
        self.ctx = osdp.PeripheralDevice(pd_info.get(), capabilities=pd_cap.get())
        self.ctx.set_loglevel(log_level)
        self.ctx.set_command_callback(self.command_handler)
        self.event = None
        self.lock = None
        self.thread = None

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

    def notify_event(self, event):
        self.lock.acquire()
        ret = self.ctx.notify_event(event)
        self.lock.release()
        return ret

    def register_file_ops(self, fops):
        self.lock.acquire()
        ret = self.ctx.register_file_ops(0, fops)
        self.lock.release()
        return ret

    def is_sc_active(self):
        return self.ctx.is_sc_active()

    def is_online(self):
        return self.ctx.is_online()

    def start(self):
        if self.thread:
            raise RuntimeError("Thread already running!")
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.ctx,)
        self.thread = threading.Thread(name='pd', target=self.refresh, args=args)
        self.thread.start()

    def stop(self):
        if not self.thread:
            raise RuntimeError("Thread not running!")
        while self.thread.is_alive():
            self.event.set()
            self.thread.join(2)
            if not self.thread.is_alive():
                self.thread = None
                break

    def teardown(self):
        self.stop()
        self.ctx = None