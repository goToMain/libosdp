from collections import namedtuple
import time
#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

from .constants import Capability, LibFlag

class PDInfo:
    def __init__(self, address: int, scbk: bytes=None, flags=[], name: str='chn'):
        self.address = address
        self.flags = flags
        self.scbk = scbk
        self.channel_device = '/tmp/pyosdp-' + name
        self.channel_speed = 115200
        self.channel_type = 'fifo'
        self.version= 1
        self.model= 1
        self.vendor_code= 0xCAFEBABE
        self.serial_number= 0xDEADBEAF
        self.firmware_version= 0x0000F00D

    def get_flags(self) -> int:
        ret = 0
        for flag in self.flags:
            ret |= flag
        return ret

    def get(self) -> dict:
        return {
            'address': self.address,
            'flags': self.get_flags(),
            'scbk': self.scbk,
            'channel_type': self.channel_type,
            'channel_speed': self.channel_speed,
            'channel_device': self.channel_device,

            # Following are needed only for PD. For CP these are don't cares
            'version': self.version,
            'model': self.model,
            'vendor_code': self.vendor_code,
            'serial_number': self.serial_number,
            'firmware_version': self.firmware_version
        }

class _PDCapEntity:
    def __init__(self, function_code: Capability,
                 compliance_level: int=1, num_items: int=1):
        self.function_code = function_code
        self.compliance_level = compliance_level
        self.num_items = num_items

    def get(self) -> dict:
        return {
            'function_code': self.function_code,
            'compliance_level': self.compliance_level,
            'num_items': self.num_items
        }

class PDCapabilities:
    def __init__(self, cap_list):
        self.capabilities = {}
        for cap in cap_list:
            (fc, cl, ni) = cap
            self.capabilities[fc] = _PDCapEntity(fc, cl, ni)

    def get(self):
        ret = []
        for fc in self.capabilities.keys():
            ret.append(self.capabilities[fc].get())
        return ret
