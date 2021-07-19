#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest
import copy

from pyosdp import *

keystore = KeyStore('/tmp')
keyname = 'pd0.key'
timeout_seconds = 5

pd_info = {
    'address': 101,
    'flags': Flag.EnforceSecure,
    'scbk': keystore.load_key(keyname, create=True),
    'channel_type': 'message_queue',
    'channel_speed': 115200,
    'channel_device': '/tmp/pyosdp-cmd-test-mq-0',

    # Following are needed only for PD. For CP these are don't cares
    'version': 1,
    'model': 1,
    'vendor_code': 0xCAFEBABE,
    'serial_number': 0xDEADBEAF,
    'firmware_version': 0x0000F00D
}

pd_cap = [
    {
        "function_code": Capability.OutputControl,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": Capability.LEDControl,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": Capability.AudiableControl,
        "compliance_level": 1,
        "num_items": 1
    },
    {
        "function_code": Capability.TextOutput,
        "compliance_level": 1,
        "num_items": 1
    },
]

pd = PeripheralDevice(pd_info, pd_cap)
cp = ControlPanel([ pd_info ])

@pytest.fixture(scope='module', autouse=True)
def setup_test():
    pd.start()
    cp.start()
    yield
    teardown_test()

def teardown_test():
    cp.stop()
    pd.stop()

def test_command_output():
    test_cmd = {
        'command': Command.Output,
        'output_no': 0,
        'control_code': 1,
        'timer_count': 10
    }
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
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
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
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
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
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
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
    assert cmd == test_cmd

def test_command_comset():
    test_cmd = {
        'command': Command.Comset,
        'address': 101,
        'baud_rate': 9600
    }
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
    assert cmd == test_cmd

def test_command_mfg():
    test_cmd = {
        'command': Command.Manufacturer,
        'vendor_code': 0x00030201,
        'mfg_command': 13,
        'data': bytes([9,1,9,2,6,3,1,7,7,0])
    }
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
    assert cmd == test_cmd

def test_command_keyset_sc():
    test_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
    }
    assert cp.send_command(101, test_cmd)
    cmd = pd.get_command(timeout_seconds)
    assert cmd == test_cmd

def test_command_keyset_plain_text():
    _pd_info = copy.deepcopy(pd_info)
    _pd_info['scbk'] = None
    _pd_info['flags'] = 0
    _pd_info['channel_device'] = '/tmp/pyosdp-cmd-test-mq-1'
    _pd = PeripheralDevice(_pd_info, [])
    _cp = ControlPanel([ _pd_info ])
    _pd.start()
    _cp.start()

    test_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
    }
    assert _cp.send_command(101, test_cmd) == False

    _cp.stop()
    _pd.stop()