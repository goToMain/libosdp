#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import time
import random
import pytest

from testlib import *

sender_data = [ random.randint(0, 255) for _ in range(4096) ]

def sender_open(file_id: int, file_size: int) -> int:
    assert file_id == 13
    assert file_size == 0 # sender has to return file_size so this must be 0
    return 4096 # we'll just send 4k of random data

def sender_read(size: int, offset: int) -> bytes:
    assert offset < 4096
    if offset + size > 4096:
        size = 4096 - offset
    return bytes(sender_data[offset:offset+size])

def sender_write(data: bytes, offset: int) -> int:
    # sender should not try to write anything!
    assert False

def sender_close(file_id: int):
    assert file_id == 13

sender_fops = {
    'open': sender_open,
    'read': sender_read,
    'write': sender_write,
    'close': sender_close
}

receiver_data = [0] * 4096

def receiver_open(file_id: int, file_size: int) -> int:
    assert file_id == 13
    assert file_size == 4096
    return 0 # indicates success. Both CP and PD have the file_size now.

def receiver_read(size: int, offset: int) -> bytes:
    # receiver should not read anything
    assert False

def receiver_write(data: bytes, offset: int) -> int:
    global receiver_data
    assert offset + len(data) <= 4096
    receiver_data[offset:offset + len(data)] = list(data)
    return len(data)

def receiver_close(file_id: int):
    assert file_id == 13

receiver_fops = {
    'open': receiver_open,
    'read': receiver_read,
    'write': receiver_write,
    'close': receiver_close
}

pd_cap = PDCapabilities([])

pd_info_list = [
    PDInfo(101, scbk=KeyStore.gen_key(), flags=[ LibFlag.EnforceSecure ], name='chn-0'),
]

pd = PeripheralDevice(pd_info_list[0], pd_cap, log_level=LogLevel.Debug)

cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)

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

def test_file_transfer(utils):
    # Register file OPs and kick off a transfer
    assert cp.register_file_ops(101, sender_fops)
    assert pd.register_file_ops(receiver_fops)
    file_tx_cmd = {
        'command': Command.FileTransfer,
        'id': 13,
        'flags': 0
    }
    assert cp.send_command(101, file_tx_cmd)
    assert pd.get_command() == file_tx_cmd

    # Monitor transfer status
    file_tx_status = False
    tries = 0
    while tries < 10:
        time.sleep(0.5)
        status = cp.get_file_tx_status(101)
        if not status or 'size' not in status or 'offset' not in status:
            break
        if status['size'] <= 0:
            break
        if status['size'] == status['offset']:
            file_tx_status = True
            break
        tries += 1
    assert file_tx_status

    # Check if the data was sent properly
    assert sender_data == receiver_data

def test_file_tx_abort(utils):
    # Register file OPs and kick off a transfer
    assert cp.register_file_ops(101, sender_fops)
    assert pd.register_file_ops(receiver_fops)
    file_tx_cmd = {
        'command': Command.FileTransfer,
        'id': 13,
        'flags': 0
    }
    assert cp.send_command(101, file_tx_cmd)
    assert pd.get_command() == file_tx_cmd

    # Allow some number of transfers to go through
    time.sleep(0.5)

    file_tx_abort = {
        'command': Command.FileTransfer,
        'id': 13,
        'flags': CommandFileTxFlags.Cancel
    }
    assert cp.send_command(101, file_tx_abort)

    # Allow some time for CP to send the abort to PD
    time.sleep(0.2)

    status = cp.get_file_tx_status(101)
    assert status == None
