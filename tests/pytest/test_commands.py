#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import pytest

import osdp
from pyosdp.control_panel import ControlPanel
from pyosdp.peripheral_device import PeripheralDevice

pd_addres = [ 101 ]

class OSDPTestCommand:
    TIMEOUT = 5
    output = {
        "command": osdp.CMD_OUTPUT,
        "output_no": 0,
        "control_code": 1,
        "timer_count": 10
    }
    buzzer = {
        "command": osdp.CMD_BUZZER,
        "reader": 0,
        "control_code": 1,
        "on_count": 10,
        "off_count": 10,
        "rep_count": 10
    }
    text = {
        "command": osdp.CMD_TEXT,
        "reader": 0,
        "control_code": 1,
        "temp_time": 20,
        "offset_row": 1,
        "offset_col": 1,
        "data": "PYOSDP"
    }
    led = {
        "command": osdp.CMD_LED,
        "reader": 1,
        "led_number": 0,
        "control_code": 1,
        "on_count": 10,
        "off_count": 10,
        "on_color": osdp.LED_COLOR_RED,
        "off_color": osdp.LED_COLOR_NONE,
        "timer_count": 10,
        "temporary": True
    }
    comset = {
        "command": osdp.CMD_COMSET,
        "address": pd_addres[0],
        "baud_rate": 9600
    }
    keyset = {
        "command": osdp.CMD_KEYSET,
        "type": 1,
        "data": bytes([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
    }
    mfg = {
        "command": osdp.CMD_MFG,
        "vendor_code": 0x00030201,
        "mfg_command": 13,
        "data": bytes([9,1,9,2,6,3,1,7,7,0])
    }

pd = PeripheralDevice(pd_addres[0])
cp = ControlPanel(pd_addres)

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
    test_cmd = OSDPTestCommand.output
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_buzzer():
    test_cmd = OSDPTestCommand.output
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_text():
    test_cmd = OSDPTestCommand.text
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_led():
    test_cmd = OSDPTestCommand.led
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_comset():
    test_cmd = OSDPTestCommand.comset
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_mfg():
    test_cmd = OSDPTestCommand.mfg
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)

def test_command_keyset():
    test_cmd = OSDPTestCommand.keyset
    cp.send_command(pd_addres[0], test_cmd)
    cmd = pd.get_command(OSDPTestCommand.TIMEOUT)
    assert(cmd == test_cmd)
