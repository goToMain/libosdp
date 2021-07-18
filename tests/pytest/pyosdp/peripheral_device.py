#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp
import time
import queue
import threading

from .key_store import KeyStore
from .helpers import osdp_refresh

class PeripheralDevice():
    def __init__(self, address):
        osdp.set_loglevel(7)
        self.keystore = KeyStore('/tmp/')
        self.address = address

        key = self.keystore.load_key('pd-' + str(address), create=True)
        pd_info = {
            "address": address,
            "flags": 0, #osdp.FLAG_ENFORCE_SECURE,
            "scbk": key,
            "channel_type": "message_queue",
            "channel_speed": 115200,
            "channel_device": '/tmp/pyosdp_mq',

            # PD_ID
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
            {
                "function_code": osdp.CAP_READER_LED_CONTROL,
                "compliance_level": 1,
                "num_items": 1
            },
            {
                "function_code": osdp.CAP_READER_AUDIBLE_OUTPUT,
                "compliance_level": 1,
                "num_items": 1
            },
            {
                "function_code": osdp.CAP_READER_TEXT_OUTPUT,
                "compliance_level": 1,
                "num_items": 1
            },
        ]
        self.command_queue = queue.Queue()
        self.pd_ctx = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)
        self.pd_ctx.set_command_callback(self.command_handler)
        self.event = threading.Event()
        self.lock = threading.Lock()
        args = (self.event, self.lock, self.pd_ctx,)
        self.thread = threading.Thread(name='pd', target=osdp_refresh, args=args)

    def command_handler(self, command):
        if command['command'] == osdp.CMD_KEYSET and command['type'] == 1:
                self.keystore.store_key('pd-' + str(self.address), command['data'])
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
