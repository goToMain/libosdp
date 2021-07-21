#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import tempfile
import osdp
import time
import queue
import threading

from .helpers import PDInfo
from .constants import LogLevel

class ControlPanel():
    def __init__(self, pd_info_list: list[PDInfo],
                 log_level: LogLevel=LogLevel.Info):
        self.pd_addr = []
        info_list = []
        for pd_info in pd_info_list:
            self.pd_addr.append(pd_info.address)
            info_list.append(pd_info.get())
        self.event_queue = [ queue.Queue() for i in self.pd_addr ]
        self.ctx = osdp.ControlPanel(info_list)
        self.ctx.set_event_callback(self.event_handler)
        self.ctx.set_loglevel(log_level)
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.ctx,)
        self.thread = threading.Thread(name='cp', target=self.refresh, args=args)

    @staticmethod
    def refresh(event, lock, ctx):
        while not event.is_set():
            lock.acquire()
            ctx.refresh()
            lock.release()
            time.sleep(0.020) #sleep for 20ms

    def event_handler(self, address, event):
        pd = self.pd_addr.index(address)
        self.event_queue[pd].put(address, event)

    def get_num_online(self):
        online = 0
        for i in range(len(self.pd_addr)):
            self.lock.acquire()
            if self.ctx.is_online(i):
                online += 1
            self.lock.release()
        return online

    def get_num_sc_active(self):
        sc_active = 0
        self.lock.acquire()
        for i in range(len(self.pd_addr)):
            if self.ctx.sc_active(i):
                sc_active += 1
        self.lock.release()
        return sc_active

    def is_sc_active(self, address):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.is_sc_active(pd)
        self.lock.release()
        return ret

    def is_online(self, address):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.is_online(pd)
        self.lock.release()
        return ret

    def send_command(self, address, cmd):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.send_command(pd, cmd)
        self.lock.release()
        return ret

    def start(self):
        if not self.thread:
            return False
        self.thread.start()
        tries = 10
        while tries:
            time.sleep(1)
            if self.get_num_online() == len(self.pd_addr):
                break
            tries -= 1
        return tries > 0

    def stop(self):
        while self.thread and self.thread.is_alive():
            self.event.set()
            self.thread.join(2)
            if not self.thread.is_alive():
                return True
        return False