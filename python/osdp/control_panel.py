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

from .helpers import PDInfo, PdId
from .constants import Capability, LibFlag, LogLevel

class ControlPanel():
    def __init__(
            self,
            pd_info_list: list[PDInfo],
            log_level: LogLevel=LogLevel.Info,
            event_handler: Callable[[int, dict], int]=None
        ) -> None:
        self.pd_addr = []
        info_list = []
        self.num_pds = len(pd_info_list)
        for pd_info in pd_info_list:
            self.pd_addr.append(pd_info.address)
            info_list.append(pd_info.get())
        self.event_queue = [ queue.Queue() for i in self.pd_addr ]
        self.user_event_handler = None
        osdp_sys.set_loglevel(log_level)
        self.ctx = osdp_sys.ControlPanel(info_list)
        # Always use our internal handler to ensure queue functionality
        self.ctx.set_event_callback(self._internal_event_handler)
        self.set_event_handler(event_handler)
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

    def set_event_handler(self, handler: Callable[[int, dict], int]):
        """Set user event handler while maintaining queue functionality"""
        self.user_event_handler = handler

    def _internal_event_handler(self, pd, event) -> int:
        """Internal handler that manages both queue and user callback"""
        # Always put event in queue for get_event() compatibility
        self.event_queue[pd].put(event)

        # If user has set a custom handler, call it too
        if self.user_event_handler:
            try:
                return self.user_event_handler(pd, event)
            except Exception as e:
                print(f"Error in user event handler: {e}")
                return -1

        return 0

    def get_event(self, address, timeout: int=5):
        pd = self.pd_addr.index(address)
        block = timeout >= 0
        try:
            event = self.event_queue[pd].get(block, timeout=timeout)
        except queue.Empty:
            return None
        return event

    def status(self):
        self.lock.acquire()
        bitmask = self.ctx.status()
        self.lock.release()
        return bitmask

    def is_online(self, address):
        pd = self.pd_addr.index(address)
        return bool(self.status() & (1 << pd))

    def get_pd_id(self, address: int) -> PdId:
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        pd_id_dict = self.ctx.get_pd_id(pd)
        self.lock.release()
        if pd_id_dict:
            # version: int, model: int, vendor_code: int, serial_number: int, firmware_version: int
            pd_id = PdId(
                pd_id_dict['version'],
                pd_id_dict['model'],
                pd_id_dict['vendor_code'],
                pd_id_dict['serial_number'],
                pd_id_dict['firmware_version']
            )
        return pd_id

    def check_capability(self, address: int, cap: Capability) -> Tuple[int, int]:
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        compliance_level, num_items = self.ctx.check_capability(pd, cap)
        self.lock.release()
        return (compliance_level, num_items)

    def get_num_online(self):
        online = 0
        bitmask = self.status()
        for i in range(len(self.pd_addr)):
            if bitmask & (1 << i):
                online += 1
        return online

    def sc_status(self):
        self.lock.acquire()
        bitmask = self.ctx.sc_status()
        self.lock.release()
        return bitmask

    def is_sc_active(self, address):
        pd = self.pd_addr.index(address)
        return bool(self.sc_status() & (1 << pd))

    def get_num_sc_active(self):
        sc_active = 0
        bitmask = self.sc_status()
        for i in range(len(self.pd_addr)):
            if bitmask & (1 << i):
                sc_active += 1
        return sc_active

    def submit_command(self, address, cmd):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.submit_command(pd, cmd)
        self.lock.release()
        return ret

    def send_command(self, address, cmd):
        from warnings import warn
        warn("This method has been renamed to submit_command", DeprecationWarning, 2)
        return self.submit_command(address, cmd)

    def set_flag(self, address, flag: LibFlag):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.set_flag(pd, flag)
        self.lock.release()
        return ret

    def clear_flag(self, address, flag: LibFlag):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.clear_flag(pd, flag)
        self.lock.release()
        return ret

    def disable_pd(self, address: int) -> bool:
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.disable_pd(pd)
        self.lock.release()
        return ret

    def enable_pd(self, address: int) -> bool:
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.enable_pd(pd)
        self.lock.release()
        return ret

    def is_pd_enabled(self, address: int) -> bool:
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.is_pd_enabled(pd)
        self.lock.release()
        return ret

    def register_file_ops(self, address, fops):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.register_file_ops(pd, fops)
        self.lock.release()
        return ret

    def get_file_tx_status(self, address):
        pd = self.pd_addr.index(address)
        self.lock.acquire()
        ret = self.ctx.get_file_tx_status(pd)
        self.lock.release()
        return ret

    def start(self):
        if self.thread:
            raise RuntimeError("Thread already running!")
        self.event = threading.Event()
        self.lock = threading.Lock()
        args=(self.event, self.lock, self.ctx)
        self.thread = threading.Thread(name='cp', target=self.refresh, args=args)
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

    def online_wait_all(self, timeout=10):
        count = 0
        res = False
        while count < timeout * 2:
            time.sleep(0.5)
            if self.get_num_online() == len(self.pd_addr):
                res = True
                break
            count += 1
        return res

    def online_wait(self, address, timeout=8):
        count = 0
        res = False
        while count < timeout * 2:
            time.sleep(0.5)
            if self.is_online(address):
                res = True
                break
            count += 1
        return res

    def offline_wait(self, address, timeout=8):
        count = 0
        res = False
        while count < timeout * 2:
            time.sleep(0.5)
            if not self.is_online(address):
                res = True
                break
            count += 1
        return res

    def sc_wait(self, address, timeout=5):
        count = 0
        res = False
        while count < timeout * 2:
            time.sleep(0.5)
            if self.is_sc_active(address):
                res = True
                break
            count += 1
        return res

    def sc_wait_all(self, timeout=5):
        count = 0
        res = False
        all_pd_mask = (1 << self.num_pds) - 1
        while count < timeout * 2:
            time.sleep(0.5)
            if self.sc_status() == all_pd_mask:
                res = True
                break
            count += 1
        return res

    def teardown(self):
        self.stop()
        self.ctx = None
