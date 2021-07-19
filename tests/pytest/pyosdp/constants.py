#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp

class Flag:
    EnforceSecure = osdp.FLAG_ENFORCE_SECURE
    InstallMode = osdp.FLAG_INSTALL_MODE

class Command:
    Output = osdp.CMD_OUTPUT
    Buzzer = osdp.CMD_BUZZER
    LED = osdp.CMD_LED
    Comset = osdp.CMD_COMSET
    Text = osdp.CMD_TEXT
    Manufacturer = osdp.CMD_MFG
    Keyset = osdp.CMD_KEYSET
    FileTransfer = osdp.CMD_FILE_TX

class CommandLEDColor:
    Black = osdp.LED_COLOR_NONE
    Red = osdp.LED_COLOR_RED
    Green = osdp.LED_COLOR_GREEN
    Amber = osdp.LED_COLOR_AMBER
    Blue = osdp.LED_COLOR_BLUE

class Event:
    CardRead = osdp.EVENT_CARDREAD
    KeyPress = osdp.EVENT_KEYPRESS
    ManufacturerReply = osdp.EVENT_MFGREP

class CardFormat:
    Unspecified = osdp.CARD_FMT_RAW_UNSPECIFIED
    Wiegand = osdp.CARD_FMT_RAW_WIEGAND
    ASCII = osdp.CARD_FMT_ASCII

class Capability:
    Unused = osdp.CAP_UNUSED
    ContactStatusMonitoring = osdp.CAP_CONTACT_STATUS_MONITORING
    OutputControl = osdp.CAP_OUTPUT_CONTROL
    CardDataFormat = osdp.CAP_CARD_DATA_FORMAT
    LEDControl = osdp.CAP_READER_LED_CONTROL
    AudiableControl = osdp.CAP_READER_AUDIBLE_OUTPUT
    TextOutput = osdp.CAP_READER_TEXT_OUTPUT
    TimeKeeping = osdp.CAP_TIME_KEEPING
    CheckCharacter = osdp.CAP_CHECK_CHARACTER_SUPPORT
    CommunicationSecurity = osdp.CAP_COMMUNICATION_SECURITY
    ReceiveBufferSize = osdp.CAP_RECEIVE_BUFFERSIZE
    CombinedMessageSize = osdp.CAP_LARGEST_COMBINED_MESSAGE_SIZE
    SmartCard = osdp.CAP_SMART_CARD_SUPPORT
    Readers = osdp.CAP_READERS
    Biometrics = osdp.CAP_BIOMETRICS
