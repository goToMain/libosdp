Packet Structure
================

This document describes how the OSDP packets are built/decoded.

Header
------

All OSDP packets must have the following 5 bytes.

+--------+------------+----------------------------------------+---------------+
| Byte   | Name       | Meaning                                | Value         |
+========+============+========================================+===============+
| 0      | SOM        | Start of Message                       | 0x53          |
+--------+------------+----------------------------------------+---------------+
| 1      | ADDR       | Physical Address of the PD             | 0x00 – 0x7E   |
+--------+------------+----------------------------------------+---------------+
| 2      | LEN\_LSB   | Packet Length Least Significant Byte   | Any           |
+--------+------------+----------------------------------------+---------------+
| 3      | LEN\_MSB   | Packet Length Most Significant Byte    | Any           |
+--------+------------+----------------------------------------+---------------+
| 4      | CTRL       | Message Control Information            | See Below     |
+--------+------------+----------------------------------------+---------------+

Message Control Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~

CTRL byte at offset 4 of packet header is a bit mask which is exploded
as:

+---------+--------+--------------+---------------------------------------------+
| BIT     | MASK   | NAME         | Meaning                                     |
+=========+========+==============+=============================================+
| 0 - 1   | 0x03   | SQN          | Packet sequence number                      |
+---------+--------+--------------+---------------------------------------------+
| 2       | 0x04   | CKSUM/ CRC   | Set: 16-bit CRC; Clear: 8-bit CHECKSUM      |
+---------+--------+--------------+---------------------------------------------+
| 3       | 0x08   | SCB          | Set: SCB is present; Clear: SCB is absent   |
+---------+--------+--------------+---------------------------------------------+

SCB is Security Control Block presence indicator. This is an optional
message block that is present in secure channel messages.

Security Control Block (Optional)
---------------------------------

+-----------+------------------+------------------------------------+-----------------+
| Byte      | Name             | Meaning                            | Value           |
+===========+==================+====================================+=================+
| 5         | SEC\_BLK\_LEN    | Length of Security Control Block   | Any             |
+-----------+------------------+------------------------------------+-----------------+
| 6         | SEC\_BLK\_TYPE   | Security Block Type                | See Below       |
+-----------+------------------+------------------------------------+-----------------+
| 7 - m-1   | SEC\_BLK\_DATA   | Security Block Data                | Based on type   |
+-----------+------------------+------------------------------------+-----------------+

Command/Reply Structure
-----------------------

The Command/Reply block is a mandatory field. If no SCB exist it follows
directly after fixed header. If SCB is present, then this is offset by as
many bytes.

+---------+--------------+-------------------------+----------------------+
| Byte    | Name         | Meaning                 | Value                |
+=========+==============+=========================+======================+
| 5       | CMND/REPLY   | Command or Reply Code   | See CMD/REPLY        |
+---------+--------------+-------------------------+----------------------+
| 6 - n   | DATA         | (optional) Data Block   | Based on CMD/REPLY   |
+---------+--------------+-------------------------+----------------------+

Message Authentication Code (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is an optional block present in secure channel messages. These 4 bytes
are just above the CRC/CKSUM bytes.

+--------+----------+--------------------------------------+---------+
| Byte   | Name     | Meaning                              | Value   |
+========+==========+======================================+=========+
| n-6    | MAC[0]   | Message Authentication Code Byte-0   | Any     |
+--------+----------+--------------------------------------+---------+
| n-5    | MAC[0]   | Message Authentication Code Byte-0   | Any     |
+--------+----------+--------------------------------------+---------+
| n-4    | MAC[0]   | Message Authentication Code Byte-0   | Any     |
+--------+----------+--------------------------------------+---------+
| n-3    | MAC[0]   | Message Authentication Code Byte-0   | Any     |
+--------+----------+--------------------------------------+---------+

Packet Validation
-----------------

These are the last 2 bytes (1 if CKSUM is was used in Message Control Information
bit mask) of the packet.

+--------+------------------+-----------------------------------------------+-------------+
| Byte   | Name             | Meaning                                       | Value       |
+========+==================+===============================================+=============+
| n-2    | CKSUM/CRC\_LSB   | Checksum, or, CRC-16 Least Significant Byte   | CKSUM/CRC   |
+--------+------------------+-----------------------------------------------+-------------+
| n-1    | CRC\_MSB         | (optional) CRC-16 Most Significant Byte       | CKSUM/CRC   |
+--------+------------------+-----------------------------------------------+-------------+
