PD Capabilities
===============

This section explodes the structure of OSDP capability response
(REPLY\_PDCAP).

Message Structure
-----------------

The REPLY\_PDCAP response is a set of 3 bytes, with the following
positional meaning.

+--------+--------------------+-------------------------------------------+-------------+
| Byte   | Name               | Meaning                                   | Value       |
+========+====================+===========================================+=============+
| 0      | Function Code      | Function/feature code                     | See Below   |
+--------+--------------------+-------------------------------------------+-------------+
| 1      | Compliance Level   | Level of compliance with above function   | See Below   |
+--------+--------------------+-------------------------------------------+-------------+
| 2      | Number of Units    | Number of objects of this type            | See Below   |
+--------+--------------------+-------------------------------------------+-------------+

Function Code 1 - Contact Status Monitoring
-------------------------------------------

This function indicates the ability to monitor the status of a switch
using a two-wire electrical connection between the PD and the switch.
The on/off position of the switch indicates the state of an external
device.

The PD may simply resolve all circuit states to an open/closed status,
or it may implement supervision of the monitoring circuit. A supervised
circuit is able to indicate circuit fault status in addition to
open/closed status.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  01 - PD monitors and reports the state of the circuit without any
   supervision. The PD encodes the circuit status per its default
   interpretation of contact state to active/inactive status.
-  02 - Like 01, plus: The PD accepts configuration of the encoding of
   the open/closed circuit status to the reported active/inactive
   status. (User may configure each circuit as "normally closed" or
   "normally open".)
-  03 - Like 02, plus: PD supports supervised monitoring. The operating
   mode for each circuit is determined by configuration settings.
-  04 - Like 03, plus: the PD supports custom End-Of-Line settings
   within the Manufacturer's guidelines.

The End-Of-Line circuit parameters are defined by the manufacturer of
the PD.

Number of Units:
~~~~~~~~~~~~~~~~

The number of Inputs.

Function Code 2 - Output Control
--------------------------------

This function provides a switched output, typically in the form of a
relay. The Output has two states: active or inactive. The Control Panel
(CP) can directly set the Output's state, or, if the PD supports timed
operations, the CP can specify a time period for the activation of the
Output.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  01 - The PD is able to activate and deactivate the Output per direct
   command from the CP.
-  02 - Like 01, plus: The PD is able to accept configuration of the
   Output driver to set the inactive state of the Output. The typical
   state of an inactive Output is the state of the Output when no power
   is applied to the PD and the Output device (relay) is not energized.
   The inverted drive setting causes the PD to energize the Output
   during the inactive state and de-energize the Output during the
   active state.
   This feature allows the support of "fail-safe/fail-secure" operating
   modes.
-  03 - Like 01, plus: The PD is able to accept timed commands to the
   Output. A timed command specifies the state of the Output for the
   specified duration.
-  04 - Like 02 and 03 - normal/inverted drive and timed operation.

Number of Units:
~~~~~~~~~~~~~~~~

The number of Outputs.

Function Code 3 - Card Data Format
----------------------------------

This capability indicates the form of the card data is presented to the
Control Panel.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  01 - the PD sends card data to the CP as array of bits, not exceeding
   1024 bits.
-  02 - the PD sends card data to the CP as array of BCD characters, not
   exceeding 256 characters.
-  03 - the PD can send card data to the CP as array of bits, or as an
   array of BCD characters.

Number of Units:
~~~~~~~~~~~~~~~~

N/A, set to 0.

Function Code 4 - Reader LED Control
------------------------------------

This capability indicates the presence of and type of LEDs.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  01 - the PD support on/off control only
-  02 - the PD supports timed commands
-  03 - like 02, plus bi-color LEDs
-  04 - like 02, plus tri-color LEDs

Number of Units:
~~~~~~~~~~~~~~~~

The number of LEDs per reader.

Function Code 5 - Reader Audible Output
---------------------------------------

This capability indicates the presence of and type of an Audible
Annunciator (buzzer or similar tone generator)

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  01 - the PD support on/off control only
-  02 - the PD supports timed commands

Number of Units:
~~~~~~~~~~~~~~~~

The number of audible annunciators per reader

Function Code 6 - Reader Text Output
------------------------------------

This capability indicates that the PD supports a text display emulating
character-based display terminals.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  00 - The PD has no text display support
-  01 - The PD supports 1 row of 16 characters
-  02 - the PD supports 2 rows of 16 characters
-  03 - the PD supports 4 rows of 16 characters
-  04 TBD.

Number of Units:
~~~~~~~~~~~~~~~~

Number of textual displays per reader.

Function Code 7 - Time Keeping
------------------------------

This capability indicates that the type of date and time awareness or
time keeping ability of the PD.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  00 - The PD does not support time/date functionality
-  01 - The PD understands time/date settings per Command osdp\_TDSET
-  02 - The PD is able to locally update the time and date

Number of Units:
~~~~~~~~~~~~~~~~

N/A, set to 0.

Function Code 8 - Check Character Support
-----------------------------------------

All PDs must be able to support the checksum mode. This capability
indicates if the PD is capable of supporting CRC mode.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  00 - The PD does not support CRC-16, only checksum mode.
-  01 - The PD supports the 16-bit CRC-16 mode.

Number of Units:
~~~~~~~~~~~~~~~~

N/A, set to 0.

Function Code 9 - Communication Security
----------------------------------------

This capability indicates the extent to which the PD supports
communication security (Secure Channel Communication)

Compliance Levels:
~~~~~~~~~~~~~~~~~~

This field is a bit map of the supported encryption algorithms

-  0x01 - (Bit-0) AES128 support
-  0x02 - (Bit-1) to be defined

Number of Units:
~~~~~~~~~~~~~~~~

This field is encoded to represent the key exchange capabilities

-  0x01 - (Bit-0) default AES128 key, as defined in APPENDIX D
-  0x02 - (Bit-1) to be defined

Function Code 10 - Receive BufferSize
-------------------------------------

This capability indicates the maximum size single message the PD can
receive.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

This field is the LSB of the buffer size

Number of Units:
~~~~~~~~~~~~~~~~

This field is the MSB of the buffer size

Function Code 11 - Largest Combined Message Size
------------------------------------------------

This capability indicates the maximum size multi-part message which the
PD can handle.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  This field is the LSB of the combined buffer size

Number of Units:
~~~~~~~~~~~~~~~~

This field is the MSB of the combined buffer size

Function Code 12 - Smart Card Support
-------------------------------------

This capability indicates whether the PD supports the transparent mode
used for communicating directly with a smart card.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  0 - PD does not support transparent reader mode
-  1 - PD does support transparent reader mode

Number of Units:
~~~~~~~~~~~~~~~~

unused, send 0x00

Function Code 13 - Readers
--------------------------

This capability indicates the number of credential reader devices
present. Compliance levels are bit fields to be assigned as needed.

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  0x01 - (Bit-0) 0X02 - (Bit-1)

Number of Units:
~~~~~~~~~~~~~~~~

Number of readers

Function Code 14 – Biometrics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This capability indicates the ability of the reader to handle biometric
input

Compliance Levels:
~~~~~~~~~~~~~~~~~~

-  0 - No Biometric
-  1 – Fingerprint, Template 1
-  2 – Fingerprint, Template 2
-  3 – Iris, Template 1

Number of Units:
~~~~~~~~~~~~~~~~

Number of readers
