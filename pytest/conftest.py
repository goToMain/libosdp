#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest

from pyosdp import *

class TestUtils:
    def __init__(self):
        self.ks = KeyStore()

    def create_cp(self, names, key, flag_list=[]):
        pd_info_list = []
        for name in names:
            addr = int(name.split("-")[1])
            pd_info_list.append(PDInfo(addr, scbk=key, flags=flag_list, name=name))
        cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)
        cp.start()
        return cp

    def create_pd(self, name, key, flag_list=[]):
        addr = int(name.split("-")[1])
        pd_info = PDInfo(addr, scbk=key, flags=flag_list, name=name)
        pd = PeripheralDevice(pd_info, PDCapabilities(), log_level=LogLevel.Debug)
        pd.start()
        return pd


@pytest.fixture(scope='session')
def utils():
    test_utils = TestUtils()
    return test_utils