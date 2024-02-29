#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import socket
import pytest
from osdp import *

import os
import abc
import fcntl
import errno

class FIFOChannel(Channel):
    def __init__(self, fifo_read_path: str, fifo_write_path: str) -> None:
        super().__init__()
        self.fifo_read_path = fifo_read_path
        self.fifo_write_path = fifo_write_path
        self.fifo_read_fd = -1
        self.fifo_write_fd = -1

    def _open_fifo(self) -> None:
        if not os.path.exists(self.fifo_read_path):
            os.mkfifo(self.fifo_read_path)
        if not os.path.exists(self.fifo_write_path):
            os.mkfifo(self.fifo_write_path)
        # Set non-blocking mode for reading
        self.fifo_read_fd = os.open(self.fifo_read_path, os.O_RDONLY | os.O_NONBLOCK)
        flags = fcntl.fcntl(self.fifo_read_fd, fcntl.F_GETFL)
        fcntl.fcntl(self.fifo_read_fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        # write can block
        self.fifo_write_fd = os.open(self.fifo_write_path, os.O_WRONLY)

    def read(self, max_bytes: int) -> bytes:
        if self.fifo_read_fd < 0:
            self._open_fifo()
        try:
            return os.read(self.fifo_read_fd, max_bytes)
        except OSError as e:
            if e.errno == errno.EAGAIN or e.errno == errno.EWOULDBLOCK:
                return b''  # Non-blocking read would block
            raise

    def write(self, buf: bytes) -> int:
        if self.fifo_write_fd < 0:
            self._open_fifo()
        return os.write(self.fifo_write_fd, buf)

    def flush(self) -> None:
        self.read(1024)

    def close(self) -> None:
        os.close(self.fifo_read_fd)
        os.close(self.fifo_write_fd)
        os.remove(self.fifo_read_path)
        os.remove(self.fifo_write_path)

class TestUtils:
    def __init__(self):
        self.ks = KeyStore()

    def create_cp(self, pd_info_list, sc_wait=False, online_wait=False):
        cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)
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


def make_fifo_pair(name):
    one = f'/tmp/fifo-{name}.one'
    two = f'/tmp/fifo-{name}.two'
    return FIFOChannel(one, two), FIFOChannel(two, one)

def cleanup_fifo_pair(name):
    one = f'/tmp/fifo-{name}.one'
    two = f'/tmp/fifo-{name}.two'
    os.remove(one)
    os.remove(two)

@pytest.fixture(scope='session')
def utils():
    return TestUtils()
