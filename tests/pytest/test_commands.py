#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest
import copy

from pyosdp import *

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 1, 1),
    (Capability.AudiableControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

secure_pd_addr = 101
secure_pd_info = PDInfo(secure_pd_addr, scbk=KeyStore.gen_key(), flags=[ LibFlag.EnforceSecure ], name='mq-0')
secure_pd = PeripheralDevice(secure_pd_info, pd_cap)

cp = ControlPanel([ secure_pd_info ])

@pytest.fixture(scope='module', autouse=True)
def setup_test():
    secure_pd.start()
    cp.start()
    yield
    teardown_test()

def teardown_test():
    cp.stop()
    secure_pd.stop()

def test_command_output():
    test_cmd = {
        'command': Command.Output,
        'output_no': 0,
        'control_code': 1,
        'timer_count': 10
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_buzzer():
    test_cmd = {
        'command': Command.Buzzer,
        'reader': 0,
        'control_code': 1,
        'on_count': 10,
        'off_count': 10,
        'rep_count': 10
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_text():
    test_cmd = {
        'command': Command.Text,
        'reader': 0,
        'control_code': 1,
        'temp_time': 20,
        'offset_row': 1,
        'offset_col': 1,
        'data': 'PYOSDP'
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_led():
    test_cmd = {
        'command': Command.LED,
        'reader': 1,
        'led_number': 0,
        'control_code': 1,
        'on_count': 10,
        'off_count': 10,
        'on_color': CommandLEDColor.Red,
        'off_color': CommandLEDColor.Black,
        'timer_count': 10,
        'temporary': True
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_comset():
    test_cmd = {
        'command': Command.Comset,
        'address': secure_pd_addr,
        'baud_rate': 9600
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_mfg():
    test_cmd = {
        'command': Command.Manufacturer,
        'vendor_code': 0x00030201,
        'mfg_command': 13,
        'data': bytes([9,1,9,2,6,3,1,7,7,0])
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd

def test_command_keyset():
    test_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
    }
    assert cp.send_command(secure_pd_addr, test_cmd)
    cmd = secure_pd.get_command()
    assert cmd == test_cmd