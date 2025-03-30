#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import pytest

from osdp import *
from conftest import make_fifo_pair, cleanup_fifo_pair

secure_pd_addr = 101
insecure_pd_addr = 102

f1_1, f1_2 = make_fifo_pair("secure_pd")
f2_1, f2_2 = make_fifo_pair("insecure_pd")

key = KeyStore.gen_key()

pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

pd_info_list = [
    PDInfo(secure_pd_addr, f1_1, scbk=key, flags=[ LibFlag.EnforceSecure, LibFlag.EnableNotification ]),
    PDInfo(insecure_pd_addr, f2_1, flags=[ LibFlag.EnableNotification ])
]

# TODO remove this.
secure_pd = PeripheralDevice(
    PDInfo(secure_pd_addr, f1_2, scbk=key, flags=[ LibFlag.EnforceSecure ]),
    pd_cap,
    log_level=LogLevel.Debug
)

insecure_pd = PeripheralDevice(
    PDInfo(insecure_pd_addr, f2_2),
    pd_cap,
    log_level=LogLevel.Debug
)

pd_list = [
    secure_pd,
    insecure_pd,
]

cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)

def cp_check_command_status(cmd, expected_outcome=True):
    event = {
        'event': Event.Notification,
        'type': EventNotification.Command,
        'arg0': cmd,
        'arg1': 1 if expected_outcome else 0,
    }
    while True:
        e = cp.get_event(secure_pd.address)
        if (e['event'] == Event.Notification and
            e['type'] == EventNotification.Command):
            break
    assert e == event

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
    cleanup_fifo_pair("secure_pd")
    cleanup_fifo_pair("insecure_pd")

def test_command_output():
    test_cmd = {
        'command': Command.Output,
        'output_no': 0,
        'control_code': 1,
        'timer_count': 10
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.Output)

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
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.Buzzer)

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
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.Text)

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
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.LED)

    test_cmd['temporary'] = False
    del test_cmd['timer_count']

    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.LED)

def test_command_comset():
    test_cmd = {
        'command': Command.Comset,
        'address': secure_pd_addr,
        'baud_rate': 9600
    }
    test_cmd_done = {
        'command': Command.ComsetDone,
        'address': secure_pd_addr,
        'baud_rate': 9600
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    assert secure_pd.get_command() == test_cmd_done
    cp_check_command_status(Command.Comset)

def test_command_mfg():
    test_cmd = {
        'command': Command.Manufacturer,
        'vendor_code': 0x00030201,
        'mfg_command': 13,
        'data': bytes([9,1,9,2,6,3,1,7,7,0])
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.Manufacturer)

def test_command_keyset():
    test_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': KeyStore.gen_key()
    }
    assert cp.is_online(secure_pd_addr)
    assert cp.submit_command(secure_pd_addr, test_cmd)
    assert secure_pd.get_command() == test_cmd
    cp_check_command_status(Command.Keyset)

    # PD must be online and SC active after a KEYSET command
    time.sleep(0.5)
    assert cp.is_online(secure_pd_addr)
    assert cp.is_sc_active(secure_pd_addr)

    # When not in SC, KEYSET command should not be accepted.
    assert cp.is_sc_active(insecure_pd_addr) == False
    assert cp.submit_command(insecure_pd_addr, test_cmd) == False

@pytest.mark.skip(
    reason=(
        "Switching callback handlers at runtime is broken;"
        " Also asserting from callback doesn't work"
    )
)
def test_command_status():
    def evt_handler(pd, event):
        assert event['event'] == Event.Status
        assert event['type'] == StatusReportType.Input
        assert event['report'] == bytes([ 0, 1, 0, 1, 0, 1, 0, 1 ])
        return 0

    def cmd_handler(command):
        assert command['command'] == Command.Status
        assert command['type'] == StatusReportType.Input
        cmd = {
            'command': Command.Status,
            'type': StatusReportType.Input,
            'report': bytes([ 0, 1, 0, 1, 0, 1, 0, 1 ])
        }
        return 0, cmd

    assert cp.is_online(secure_pd_addr)
    cp.set_event_handler(evt_handler)
    secure_pd.set_command_handler(cmd_handler)

    test_cmd = {
        'command': Command.Status,
        'type': StatusReportType.Input,
        'report': bytes([]),
    }
    cp.submit_command(secure_pd_addr, test_cmd)
