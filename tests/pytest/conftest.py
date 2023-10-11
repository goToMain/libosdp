#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest

from testlib import *

class TestUtils:
    def __init__(self):
        self.ks = KeyStore()

    def create_cp(self, pd_info_list, sc_wait=False, online_wait=False,
                  master_key=None):
        cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug, master_key=master_key)
        cp.start()
        if sc_wait:
            assert cp.sc_wait_all()
        elif online_wait:
            assert cp.online_wait_all()
        return cp

    def create_pd(self, pd_info):
        pd = PeripheralDevice(pd_info, PDCapabilities(), log_level=LogLevel.Debug)
        pd.start()
        return pd


@pytest.fixture(scope='session')
def utils():
    test_utils = TestUtils()
    return test_utils