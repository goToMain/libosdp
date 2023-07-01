#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest

from testlib import *

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 8),
    (Capability.ContactStatusMonitoring, 1, 8),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

pd_info_list = [
    PDInfo(101, scbk=KeyStore.gen_key(), flags=[ LibFlag.EnforceSecure ], name='chn-0'),
]

secure_pd = PeripheralDevice(pd_info_list[0], pd_cap, log_level=LogLevel.Debug)

pd_list = [
    secure_pd,
]

cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)

@pytest.fixture(scope='module', autouse=True)
def setup_test():
    for pd in pd_list:
        pd.start()
    cp.start()
    cp.online_wait_all()
    yield
    teardown_test()

def teardown_test():
    cp.teardown()
    for pd in pd_list:
        pd.teardown()

def test_event_keypad():
    event = {
        'event': Event.KeyPress,
        'reader_no': 1,
        'data': bytes([9,1,9,2,6,3,1,7,7,0]),
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_mfg_reply():
    event = {
        'event': Event.ManufacturerReply,
        'vendor_code': 0x153,
        'mfg_command': 0x10,
        'data': bytes([9,1,9,2,6,3,1,7,7,0]),
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_cardread_ascii():
    event = {
        'event': Event.CardRead,
        'reader_no': 1,
        'direction': 1,
        'format': CardFormat.ASCII,
        'data': bytes([9,1,9,2,6,3,1,7,7,0]),
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_cardread_wiegand():
    event = {
        'event': Event.CardRead,
        'reader_no': 1,
        'direction': 0, # has to be zero
        'length': 16,
        'format': CardFormat.Wiegand,
        'data': bytes([0x55, 0xAA]),
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_input():
    event = {
        'event': Event.InputOutput,
        'type': 0, # 0 - input; 1 - output
        'status': 0xAA, # bit mask of input/output status (upto 32)
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_output():
    event = {
        'event': Event.InputOutput,
        'type': 1, # 0 - input; 1 - output
        'status': 0x55, # bit mask of input/output status (upto 32)
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event

def test_event_status():
    event = {
        'event': Event.Status,
        'power': 0, # 0 - normal; 1 - power failure
        'tamper': 1, # 0 - normal; 1 - tamper
    }
    secure_pd.notify_event(event)
    assert cp.get_event(secure_pd.address) == event
