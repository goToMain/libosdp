#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp

class LibFlag:
    EnforceSecure = osdp.FLAG_ENFORCE_SECURE
    InstallMode = osdp.FLAG_INSTALL_MODE
    IgnoreUnsolicited = osdp.FLAG_IGN_UNSOLICITED

class LogLevel:
    Emergency = osdp.LOG_EMERG
    Alert = osdp.LOG_ALERT
    Critical = osdp.LOG_CRIT
    Error = osdp.LOG_ERROR
    Warning = osdp.LOG_WARNING
    Notice = osdp.LOG_NOTICE
    Info = osdp.LOG_INFO
    Debug = osdp.LOG_DEBUG

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

class CommandFileTxFlags:
    Cancel = osdp.CMD_FILE_TX_FLAG_CANCEL

class Event:
    CardRead = osdp.EVENT_CARDREAD
    KeyPress = osdp.EVENT_KEYPRESS
    ManufacturerReply = osdp.EVENT_MFGREP
    InputOutput = osdp.EVENT_IO
    Status = osdp.EVENT_STATUS

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
    AudibleControl = osdp.CAP_READER_AUDIBLE_OUTPUT
    TextOutput = osdp.CAP_READER_TEXT_OUTPUT
    TimeKeeping = osdp.CAP_TIME_KEEPING
    CheckCharacter = osdp.CAP_CHECK_CHARACTER_SUPPORT
    CommunicationSecurity = osdp.CAP_COMMUNICATION_SECURITY
    ReceiveBufferSize = osdp.CAP_RECEIVE_BUFFERSIZE
    CombinedMessageSize = osdp.CAP_LARGEST_COMBINED_MESSAGE_SIZE
    SmartCard = osdp.CAP_SMART_CARD_SUPPORT
    Readers = osdp.CAP_READERS
    Biometrics = osdp.CAP_BIOMETRICS
