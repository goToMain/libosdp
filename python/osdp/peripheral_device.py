#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp_sys
import time
import queue
import threading
from typing import Callable, Tuple

from .helpers import PDInfo, PDCapabilities
from .constants import LogLevel

class PeripheralDevice():
    def __init__(self, pd_info: PDInfo, pd_cap: PDCapabilities,
                 log_level: LogLevel=LogLevel.Info,
                 command_handler: Callable[[dict], Tuple[int, dict]]=None):
        self.command_queue = queue.Queue()
        self.address = pd_info.address
        osdp_sys.set_loglevel(log_level)
        self.ctx = osdp_sys.PeripheralDevice(pd_info.get(), capabilities=pd_cap.get())
        if command_handler:
            self.ctx.set_command_callback(command_handler)
        else:
            self.ctx.set_command_callback(self.command_handler)
        self.set_command_handler(command_handler)
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

    def command_handler(self, command) -> Tuple[int, dict]:
        self.command_queue.put(command)
        return 0, None

    def set_command_handler(self, handler: Callable[[dict], Tuple[int, dict]]):
        if handler:
            self.ctx.set_command_callback(handler)
        else:
            self.ctx.set_command_callback(self.command_handler)

    def get_command(self, timeout: int=5):
        block = timeout >= 0
        try:
            cmd = self.command_queue.get(block, timeout=timeout)
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

    def sc_wait(self, timeout=5):
        count = 0
        res = False
        while count < timeout * 2:
            time.sleep(0.5)
            if self.is_sc_active():
                res = True
                break
            count += 1
        return res

    def start(self):
        if self.thread:
            raise RuntimeError("Thread already running!")
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.ctx,)
        self.thread = threading.Thread(name='pd', target=self.refresh, args=args)
        self.thread.start()

    def get_file_tx_status(self):
        self.lock.acquire()
        ret = self.ctx.get_file_tx_status(0)
        self.lock.release()
        return ret

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
