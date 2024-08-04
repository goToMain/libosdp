#
#  Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import pytest

from osdp import *
from conftest import make_fifo_pair, cleanup_fifo_pair

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

key = KeyStore.gen_key()
f1, f2 = make_fifo_pair("status")

# TODO remove this.
pd_addr = 101
pd = PeripheralDevice(PDInfo(pd_addr, f1, scbk=key), pd_cap, log_level=LogLevel.Debug)
cp = ControlPanel([PDInfo(pd_addr, f2, scbk=key)])

@pytest.fixture(scope='module', autouse=True)
def setup_test():
    pd.start()
    cp.start()
    cp.sc_wait_all()
    yield
    teardown_test()

def teardown_test():
    cp.teardown()
    pd.teardown()
    cleanup_fifo_pair("status")

def test_cp_status():
    assert cp.online_wait(pd.address)
    pd.stop()
    assert cp.online_wait(pd.address) == False
    pd.start()
    assert cp.online_wait(pd.address)

def test_cp_sc_status():
    assert cp.sc_wait(pd.address)
    pd.stop()
    assert cp.sc_wait(pd.address) == False
    pd.start()
    assert cp.sc_wait(pd.address)

def test_pd_status():
    cp.stop()
    time.sleep(1)
    assert pd.is_online() == False
    cp.start()
    assert cp.sc_wait(pd.address)
    assert pd.is_online()

def test_pd_sc_status():
    cp.stop()
    time.sleep(1)
    assert pd.is_sc_active() == False
    cp.start()
    assert cp.sc_wait(pd.address)
    assert pd.is_sc_active()
