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

# ============================================================================
# Test Helper Functions for OSDP Communication Assertions
# ============================================================================
# These helpers abstract away timeout handling and provide consistent error
# messages when OSDP communication fails during tests.

def assert_command_received(pd, expected_cmd, timeout=2):
    """Helper to assert that a PD receives the expected command within timeout

    Args:
        pd: PeripheralDevice instance
        expected_cmd: Expected command dict to match
        timeout: Timeout in seconds (default: 2)

    Returns:
        The received command dict

    Raises:
        pytest.fail if timeout occurs or command doesn't match
    """
    cmd = pd.get_command(timeout=timeout)
    if cmd is None:
        pytest.fail(f"Timeout waiting for command after {timeout}s")
    assert cmd == expected_cmd
    return cmd

def assert_event_received(cp, pd_address, expected_event, timeout=2):
    """Helper to assert that a CP receives the expected event within timeout"""
    event = cp.get_event(pd_address, timeout=timeout)
    if event is None:
        pytest.fail(f"Timeout waiting for event after {timeout}s")
    assert event == expected_event
    return event

def wait_for_notification_event(cp, pd_address, expected_event, timeout=5):
    """Helper to wait for a specific notification event, filtering out other events"""
    timeout_count = 0
    max_attempts = int(timeout * 2)  # Check every 0.5s

    while timeout_count < max_attempts:
        e = cp.get_event(pd_address, timeout=0.5)
        if e and e['event'] == Event.Notification and e['type'] == expected_event['type']:
            if e == expected_event:
                return e
        timeout_count += 1

    pytest.fail(f"Timeout waiting for notification event after {timeout}s")

def wait_for_non_notification_event(cp, pd_address, expected_event, timeout=5):
    """Helper to wait for a non-notification event, filtering out notification events"""
    timeout_count = 0
    max_attempts = int(timeout * 2)  # Check every 0.5s

    while timeout_count < max_attempts:
        e = cp.get_event(pd_address, timeout=0.5)
        if e and e['event'] != Event.Notification:
            if e == expected_event:
                return e
        timeout_count += 1

    pytest.fail(f"Timeout waiting for non-notification event after {timeout}s")

class TestUtils:
    def __init__(self):
        self.ks = KeyStore()

    def create_cp(self, pd_info_list, sc_wait=False, online_wait=False):
        cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)
        cp.start()
        if sc_wait:
            if not cp.sc_wait_all(timeout=10):
                cp.teardown()
                pytest.fail("Failed to establish secure channel within timeout")
        elif online_wait:
            if not cp.online_wait_all(timeout=10):
                cp.teardown()
                pytest.fail("Failed to bring all PDs online within timeout")
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
