Introduction
============

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP). The OSDP specification describes the
protocol implementation over a two-wire RS-485 multi-drop serial communication
channel Nevertheless, this protocol can be used to transfer secure data over any
physical channel.

LibOSDP complies with v2.2 of the OSDP specification. This page pulls excepts
from the specification that are crucial points when trying to understand the
protocol.

Physical Interface
------------------

Half-duplex RS-485 - one twisted pair, shield/signal ground

Signaling
---------

Half duplex asynchronous serial 8 data bits, 1 stop bit, no parity bits, with
either one of 9600, 19200, 38400, 57600, 115200 or 230400 baud rates.

Character Encoding
------------------

The complete 8-bit character is used. All possible bit patterns may appear
within a message.

Channel Access
--------------

The communication channel is used in the “interrogation/reply” mode. Only the CP
may spontaneously send a message. Each message sent by the CP is addressed to
one and only one PD.

Timing
------

The transmitting device shall guarantee a gap of a minimum of two character
times before it may access the communication channel. This idle line delay is
required to allow for signal converters and/or multiplexers to sense that the
line has become idle.

The PD shall send a single reply message to each message addressed to it within
200 ms. If the PD is unable to accept the command for processing due to temporary
unavailability of a resource required to process the command, then the PD shall
send the osdp_BUSY reply. When the CP receives the osdp_BUSY reply, it may, at
its discretion, choose to re-send the same command as it would if the command
delivery timed out.

The typical REPLY_DELAY should be less than 3 milliseconds. If a device is
overwhelmed it can send a BUSY message.

Message Synchronization
-----------------------

The general procedure for a peripheral device (PD) to obtain message
synchronization is to wait for an inter-character timeout then look for a
Start-Of-Message (SOM) code. The device should then receive and store at least
the header fields while computing the checksum/CRC on the rest of the message.
If the checksum is good, only the PD that matches the address field processes
the message. All other PDs, however, should monitor the packet by counting the
remaining portion of packet to be able to anticipate the start of the next
packet.

If there is an inter-character timeout while receiving the message the PD shall
abort the receive sequence. Once aborted, the PD should re-sync using the method
described above.

The nominal value of the inter-character timeout shall be 20 milliseconds. This
parameter may need to be adjusted for special channel timing considerations.

Packet Structure
----------------

See `packet structure documentation`_.

.. _packet structure documentation: packet-structure.html

Peripheral Device Capabilities
------------------------------

OSDP PDs must advertise a list of predefined capabilities to the CP in response
the osdp_PDCAP command. See a `comprehensive list PD capabilities`_.

.. _comprehensive list PD capabilities: pd-capabilities.html

for more details.

Commands
--------

See list of `commands supported by LibOSDP`_.

.. _commands supported by LibOSDP: commands-and-replies.html
