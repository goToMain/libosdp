#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest

from osdp import *
from conftest import make_fifo_pair, cleanup_fifo_pair

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 8),
    (Capability.ContactStatusMonitoring, 1, 8),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

key = KeyStore.gen_key()
f1, f2 = make_fifo_pair("events")

secure_pd = PeripheralDevice(
    PDInfo(101, f1, scbk=key, flags=[ LibFlag.EnforceSecure ]),
    pd_cap,
    log_level=LogLevel.Debug
)

pd_list = [
    secure_pd,
]

cp = ControlPanel([
        PDInfo(101, f2, scbk=key, flags=[ LibFlag.EnforceSecure, LibFlag.EnableNotification ])
    ],
    log_level=LogLevel.Debug
)

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
    cleanup_fifo_pair("events")

def check_event(event):
    while True:
        e = cp.get_event(secure_pd.address)
        if e['event'] != Event.Notification:
            break
    assert e == event

def test_event_keypad():
    event = {
        'event': Event.KeyPress,
        'reader_no': 1,
        'data': bytes([9,1,9,2,6,3,1,7,7,0]),
    }
    secure_pd.submit_event(event)
    check_event(event)

def test_event_mfg_reply():
    event = {
        'event': Event.ManufacturerReply,
        'vendor_code': 0x153,
        'mfg_command': 0x10,
        'data': bytes([9,1,9,2,6,3,1,7,7,0]),
    }
    secure_pd.submit_event(event)
    check_event(event)

def test_event_cardread_wiegand():
    event = {
        'event': Event.CardRead,
        'reader_no': 1,
        'direction': 0, # has to be zero
        'length': 16,
        'format': CardFormat.Wiegand,
        'data': bytes([0x55, 0xAA]),
    }
    secure_pd.submit_event(event)
    check_event(event)

def test_event_input():
    event = {
        'event': Event.Status,
        'type': StatusReportType.Input,
        'report': bytes([1, 0, 1, 0, 1, 0, 1, 0])
    }
    secure_pd.submit_event(event)
    check_event(event)

def test_event_output():
    event = {
        'event': Event.Status,
        'type': StatusReportType.Output,
        'report': bytes([0, 1, 0, 1, 0, 1, 0, 1])
    }
    secure_pd.submit_event(event)
    check_event(event)
