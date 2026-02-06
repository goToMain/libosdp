#
#  Copyright (c) 2026 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import pytest
from osdp import *
from conftest import *

class PacketBuffer:
    """Container for a packet buffer with reference counting"""
    def __init__(self, data: bytes):
        self.data = data
        self.released = False

class ZeroCopyChannel(Channel):
    """Zero-copy channel implementation that assembles complete OSDP packets"""

    OSDP_PKT_MARK = 0xFF
    OSDP_PKT_SOM = 0x53

    def __init__(self, fifo_read_path: str, fifo_write_path: str) -> None:
        super().__init__()
        self.fifo_read_path = fifo_read_path
        self.fifo_write_path = fifo_write_path
        self.fifo_read_fd = -1
        self.fifo_write_fd = -1

        # Packet assembly state
        self.rx_buffer = bytearray()
        self.current_packet = None
        self.last_byte = 0

        # Statistics
        self.packets_received = 0
        self.packets_released = 0
        self.bytes_received = 0

        # Open read FIFO immediately so other side can open write FIFO
        if not os.path.exists(self.fifo_read_path):
            os.mkfifo(self.fifo_read_path)
        self.fifo_read_fd = os.open(self.fifo_read_path, os.O_RDONLY | os.O_NONBLOCK)

    def _open_fifo(self) -> None:
        import time
        import errno
        import fcntl
        if not os.path.exists(self.fifo_read_path):
            os.mkfifo(self.fifo_read_path)
        if not os.path.exists(self.fifo_write_path):
            os.mkfifo(self.fifo_write_path)

        # Open read side in non-blocking mode
        if self.fifo_read_fd < 0:
            self.fifo_read_fd = os.open(self.fifo_read_path, os.O_RDONLY | os.O_NONBLOCK)

        # Open write side: use non-blocking for the open() call to avoid deadlock,
        # then make it blocking for actual I/O operations
        if self.fifo_write_fd < 0:
            # Retry opening write FIFO with timeout
            for _ in range(100):  # 10 second timeout
                try:
                    self.fifo_write_fd = os.open(self.fifo_write_path, os.O_WRONLY | os.O_NONBLOCK)
                    # Successfully opened, now make it blocking for writes
                    flags = fcntl.fcntl(self.fifo_write_fd, fcntl.F_GETFL)
                    fcntl.fcntl(self.fifo_write_fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)
                    break
                except OSError as e:
                    if e.errno == errno.ENXIO:  # No reader yet
                        time.sleep(0.1)
                        continue
                    raise
            else:
                raise TimeoutError(f"Failed to open write FIFO {self.fifo_write_path} - no reader")

    def _assemble_packet(self) -> bytes:
        """Assemble a complete OSDP packet from the RX buffer"""
        import errno

        # Read more bytes from FIFO
        if self.fifo_read_fd < 0:
            self._open_fifo()

        try:
            data = os.read(self.fifo_read_fd, 256)
            if data:
                self.rx_buffer.extend(data)
                self.bytes_received += len(data)
        except OSError as e:
            if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                raise

        # Look for packet start
        while len(self.rx_buffer) > 0:
            # Check for MARK + SOM
            if len(self.rx_buffer) >= 2 and self.rx_buffer[0] == self.OSDP_PKT_MARK and self.rx_buffer[1] == self.OSDP_PKT_SOM:
                has_mark = True
                hdr_offset = 1
            elif self.rx_buffer[0] == self.OSDP_PKT_SOM:
                has_mark = False
                hdr_offset = 0
            else:
                # Invalid start, discard byte
                self.rx_buffer.pop(0)
                continue

            # Need at least header (SOM + addr + len_lsb + len_msb + control = 5 bytes after mark)
            min_len = hdr_offset + 5
            if len(self.rx_buffer) < min_len:
                return None  # Wait for more data

            # Parse packet length from header
            len_lsb = self.rx_buffer[hdr_offset + 2]
            len_msb = self.rx_buffer[hdr_offset + 3]
            pkt_len = (len_msb << 8) | len_lsb
            pkt_len += (1 if has_mark else 0)

            # Validate packet length
            if pkt_len < min_len or pkt_len > 1024:
                # Invalid length, discard start byte and retry
                self.rx_buffer.pop(0)
                continue

            # Wait for complete packet
            if len(self.rx_buffer) < pkt_len:
                return None

            # Extract complete packet
            packet = bytes(self.rx_buffer[:pkt_len])
            self.rx_buffer = self.rx_buffer[pkt_len:]
            return packet

        return None

    def read(self, max_bytes: int) -> bytes:
        """
        Traditional read interface - not used in zero-copy mode.
        Should not be called when read_packet is available.
        """
        raise NotImplementedError("Use read_packet() for zero-copy mode")

    def read_packet(self) -> bytes:
        """
        Zero-copy interface: Return a complete OSDP packet if available.
        Returns None if no complete packet is ready.
        """
        if self.current_packet is not None:
            # Previous packet not released yet
            return self.current_packet.data

        packet = self._assemble_packet()
        if packet:
            self.current_packet = PacketBuffer(packet)
            self.packets_received += 1
            return packet

        return None

    def release_packet(self, buf: bytes) -> None:
        """
        Zero-copy interface: Release the packet buffer.
        """
        if self.current_packet is not None:
            self.current_packet.released = True
            self.current_packet = None
            self.packets_released += 1

    def write(self, buf: bytes) -> int:
        if self.fifo_write_fd < 0:
            self._open_fifo()
        return os.write(self.fifo_write_fd, buf)

    def flush(self) -> None:
        """Flush RX buffer"""
        import errno
        if self.fifo_read_fd < 0:
            return
        try:
            while True:
                data = os.read(self.fifo_read_fd, 1024)
                if not data:
                    break
        except OSError as e:
            if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                raise
        self.rx_buffer.clear()
        self.current_packet = None

    def close(self) -> None:
        if self.fifo_read_fd >= 0:
            os.close(self.fifo_read_fd)
        if self.fifo_write_fd >= 0:
            os.close(self.fifo_write_fd)
        if os.path.exists(self.fifo_read_path):
            os.remove(self.fifo_read_path)
        if os.path.exists(self.fifo_write_path):
            os.remove(self.fifo_write_path)

    def get_stats(self):
        """Get zero-copy statistics"""
        return {
            'packets_received': self.packets_received,
            'packets_released': self.packets_released,
            'bytes_received': self.bytes_received,
            'buffer_size': len(self.rx_buffer)
        }


def make_zero_copy_fifo_pair(name):
    """Create a pair of zero-copy channels for CP<->PD communication"""
    one = f'/tmp/fifo-zerocopy-{name}.one'
    two = f'/tmp/fifo-zerocopy-{name}.two'
    return ZeroCopyChannel(one, two), ZeroCopyChannel(two, one)


@pytest.mark.skipif(not hasattr(ZeroCopyChannel, 'read_packet'),
                    reason="Zero-copy support not available")
def test_zero_copy_basic_communication(utils):
    """Test basic CP-PD communication using zero-copy channels"""
    cp_channel, pd_channel = make_zero_copy_fifo_pair('basic')

    try:
        pd_cap = PDCapabilities([
            (Capability.LEDControl, 1, 1),
        ])

        pd_info = PDInfo(
            address=101,
            scbk=None,
            channel=pd_channel,
            flags=[]
        )

        pd = utils.create_pd(pd_info, pd_cap)

        cp_pd_info = PDInfo(
            address=101,
            scbk=None,
            channel=cp_channel,
            flags=[]
        )

        cp = utils.create_cp([cp_pd_info], online_wait=True)

        # Send a command from CP to PD
        test_cmd = {
            'command': Command.LED,
            'reader': 0,
            'led_number': 0,
            'control_code': 1,
            'on_count': 10,
            'off_count': 10,
            'on_color': CommandLEDColor.Red,
            'off_color': CommandLEDColor.Black,
            'temporary': False
        }
        assert cp.submit_command(101, test_cmd)

        # PD should receive the command
        cmd = assert_command_received(pd, test_cmd)

        # Verify zero-copy statistics
        cp_stats = cp_channel.get_stats()
        pd_stats = pd_channel.get_stats()

        assert cp_stats['packets_received'] > 0, "CP should receive packets via zero-copy"
        assert cp_stats['packets_released'] > 0, "CP should release packets"
        assert cp_stats['packets_released'] == cp_stats['packets_received'], "All received packets should be released"

        assert pd_stats['packets_received'] > 0, "PD should receive packets via zero-copy"
        assert pd_stats['packets_released'] > 0, "PD should release packets"
        assert pd_stats['packets_released'] == pd_stats['packets_received'], "All received packets should be released"

        print(f"\nZero-copy stats:")
        print(f"  CP: {cp_stats}")
        print(f"  PD: {pd_stats}")

    finally:
        cp.teardown()
        pd.teardown()
        cp_channel.close()
        pd_channel.close()


@pytest.mark.skipif(not hasattr(ZeroCopyChannel, 'read_packet'),
                    reason="Zero-copy support not available")
def test_zero_copy_packet_release(utils):
    """Verify that zero-copy packets are properly released"""
    cp_channel, pd_channel = make_zero_copy_fifo_pair('release')

    try:
        pd_cap = PDCapabilities([
            (Capability.OutputControl, 1, 8),
        ])

        pd_info = PDInfo(
            address=102,
            scbk=None,
            channel=pd_channel,
            flags=[]
        )

        pd = utils.create_pd(pd_info, pd_cap)

        cp_pd_info = PDInfo(
            address=102,
            scbk=None,
            channel=cp_channel,
            flags=[]
        )

        cp = utils.create_cp([cp_pd_info], online_wait=True)

        # Get initial stats
        cp_initial = cp_channel.get_stats()
        pd_initial = pd_channel.get_stats()

        # Send multiple commands
        for i in range(5):
            assert cp.submit_command(102, {
                'command': Command.Output,
                'output_no': i % 4,
                'control_code': 1,
                'timer_count': 0
            })

            cmd = assert_command_received(pd, {
                'command': Command.Output,
                'output_no': i % 4,
                'control_code': 1,
                'timer_count': 0
            })

        # Get final stats
        cp_final = cp_channel.get_stats()
        pd_final = pd_channel.get_stats()

        # Verify all packets were released
        cp_new_packets = cp_final['packets_received'] - cp_initial['packets_received']
        cp_new_releases = cp_final['packets_released'] - cp_initial['packets_released']

        assert cp_new_packets > 0, "CP should have received new packets"
        assert cp_new_releases == cp_new_packets, f"CP should release all packets: received={cp_new_packets}, released={cp_new_releases}"

        pd_new_packets = pd_final['packets_received'] - pd_initial['packets_received']
        pd_new_releases = pd_final['packets_released'] - pd_initial['packets_released']

        assert pd_new_packets >= 5, f"PD should have received at least 5 packets, got {pd_new_packets}"
        assert pd_new_releases == pd_new_packets, f"PD should release all packets: received={pd_new_packets}, released={pd_new_releases}"

    finally:
        cp.teardown()
        pd.teardown()
        cp_channel.close()
        pd_channel.close()


@pytest.mark.skipif(not hasattr(ZeroCopyChannel, 'read_packet'),
                    reason="Zero-copy support not available")
def test_zero_copy_vs_traditional(utils):
    """Compare zero-copy mode with traditional mode"""
    # Test with zero-copy
    zc_cp_channel, zc_pd_channel = make_zero_copy_fifo_pair('comparison-zc')

    # Test with traditional
    trad_cp_channel, trad_pd_channel = make_fifo_pair('comparison-trad')

    try:
        pd_cap = PDCapabilities([
            (Capability.AudibleControl, 1, 1),
        ])

        # Setup zero-copy
        zc_pd_info = PDInfo(address=103, scbk=None, channel=zc_pd_channel, flags=[])
        zc_pd = utils.create_pd(zc_pd_info, pd_cap)
        zc_cp_pd_info = PDInfo(address=103, scbk=None, channel=zc_cp_channel, flags=[])
        zc_cp = utils.create_cp([zc_cp_pd_info], online_wait=True)

        # Setup traditional
        trad_pd_info = PDInfo(address=104, scbk=None, channel=trad_pd_channel, flags=[])
        trad_pd = utils.create_pd(trad_pd_info, pd_cap)
        trad_cp_pd_info = PDInfo(address=104, scbk=None, channel=trad_cp_channel, flags=[])
        trad_cp = utils.create_cp([trad_cp_pd_info], online_wait=True)

        # Send same command to both
        test_cmd = {
            'command': Command.Buzzer,
            'reader': 0,
            'control_code': 2,  # Default tone
            'on_count': 1,
            'off_count': 1,
            'rep_count': 3
        }

        assert zc_cp.submit_command(103, test_cmd)
        assert trad_cp.submit_command(104, test_cmd)

        # Both should receive correctly
        zc_cmd = assert_command_received(zc_pd, test_cmd)
        trad_cmd = assert_command_received(trad_pd, test_cmd)

        # Commands should be identical
        assert zc_cmd == trad_cmd, "Zero-copy and traditional modes should produce identical results"

        # Verify zero-copy used packet interface
        zc_stats = zc_cp_channel.get_stats()
        assert zc_stats['packets_received'] > 0, "Zero-copy mode should use packet interface"

        print(f"\nMode comparison successful:")
        print(f"  Zero-copy packets: received={zc_stats['packets_received']}, released={zc_stats['packets_released']}")
        print(f"  Both modes produced identical results")

    finally:
        zc_cp.teardown()
        zc_pd.teardown()
        trad_cp.teardown()
        trad_pd.teardown()
        zc_cp_channel.close()
        zc_pd_channel.close()
        trad_cp_channel.close()
        trad_pd_channel.close()


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
