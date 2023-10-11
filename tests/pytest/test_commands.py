#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import pytest

from testlib import *

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

pd_info_list = [
    PDInfo(101, scbk=KeyStore.gen_key(), flags=[ LibFlag.EnforceSecure ], name='chn-0'),
    PDInfo(102, name='chn-1')
]

# TODO remove this.
secure_pd_addr = pd_info_list[0].address
secure_pd = PeripheralDevice(pd_info_list[0], pd_cap, log_level=LogLevel.Debug)

insecure_pd_addr = pd_info_list[1].address
insecure_pd = PeripheralDevice(pd_info_list[1], pd_cap, log_level=LogLevel.Debug)

pd_list = [
    secure_pd,
    insecure_pd,
]

cp = ControlPanel(pd_info_list)

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

def test_command_output():
    test_cmd = {
        'command': Command.Output,
        'output_no': 0,
        'control_code': 1,
        'timer_count': 10
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

def test_command_buzzer():
    test_cmd = {
        'command': Command.Buzzer,
        'reader': 0,
        'control_code': 1,
        'on_count': 10,
        'off_count': 10,
        'rep_count': 10
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

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
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

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
    assert secure_pd.get_command() == test_cmd

    test_cmd['temporary'] = False
    del test_cmd['timer_count']

    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

def test_command_comset():
    test_cmd = {
        'command': Command.Comset,
        'address': secure_pd_addr,
        'baud_rate': 9600
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

def test_command_mfg():
    test_cmd = {
        'command': Command.Manufacturer,
        'vendor_code': 0x00030201,
        'mfg_command': 13,
        'data': bytes([9,1,9,2,6,3,1,7,7,0])
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

def test_command_keyset():
    test_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': KeyStore.gen_key()
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.send_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd

    # PD must be online and SC active after a KEYSET command
    time.sleep(0.5)
    assert cp.is_online(secure_pd_addr)
    assert cp.is_sc_active(secure_pd_addr)

    # When not in SC, KEYSET command should not be accepted.
    assert cp.is_sc_active(insecure_pd_addr) == False
    assert cp.send_command(insecure_pd_addr, test_cmd) == False
