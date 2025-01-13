#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import osdp_sys

class LibFlag:
    EnforceSecure = osdp_sys.FLAG_ENFORCE_SECURE
    InstallMode = osdp_sys.FLAG_INSTALL_MODE
    IgnoreUnsolicited = osdp_sys.FLAG_IGN_UNSOLICITED
    EnableNotification = osdp_sys.FLAG_ENABLE_NOTIFICATION
    CapturePackets = osdp_sys.FLAG_CAPTURE_PACKETS
    AllowEmptyEncryptedDataBlock = osdp_sys.FLAG_ALLOW_EMPTY_ENCRYPTED_DATA_BLOCK

class LogLevel:
    Emergency = osdp_sys.LOG_EMERG
    Alert = osdp_sys.LOG_ALERT
    Critical = osdp_sys.LOG_CRIT
    Error = osdp_sys.LOG_ERROR
    Warning = osdp_sys.LOG_WARNING
    Notice = osdp_sys.LOG_NOTICE
    Info = osdp_sys.LOG_INFO
    Debug = osdp_sys.LOG_DEBUG

class StatusReportType:
    Local = osdp_sys.STATUS_REPORT_LOCAL
    Input = osdp_sys.STATUS_REPORT_INPUT
    Output = osdp_sys.STATUS_REPORT_OUTPUT
    Remote = osdp_sys.STATUS_REPORT_REMOTE

class Command:
    Output = osdp_sys.CMD_OUTPUT
    Buzzer = osdp_sys.CMD_BUZZER
    LED = osdp_sys.CMD_LED
    Comset = osdp_sys.CMD_COMSET
    ComsetDone = osdp_sys.CMD_COMSET_DONE
    Text = osdp_sys.CMD_TEXT
    Manufacturer = osdp_sys.CMD_MFG
    Keyset = osdp_sys.CMD_KEYSET
    FileTransfer = osdp_sys.CMD_FILE_TX
    Status = osdp_sys.CMD_STATUS

class CommandLEDColor:
    Black = osdp_sys.LED_COLOR_NONE
    Red = osdp_sys.LED_COLOR_RED
    Green = osdp_sys.LED_COLOR_GREEN
    Amber = osdp_sys.LED_COLOR_AMBER
    Blue = osdp_sys.LED_COLOR_BLUE
    Magenta = osdp_sys.LED_COLOR_MAGENTA
    Cyan = osdp_sys.LED_COLOR_CYAN
    White = osdp_sys.LED_COLOR_WHITE

class CommandFileTxFlags:
    Cancel = osdp_sys.CMD_FILE_TX_FLAG_CANCEL

class EventNotification:
    Command = osdp_sys.EVENT_NOTIFICATION_COMMAND
    SecureChannelStatus = osdp_sys.EVENT_NOTIFICATION_SC_STATUS
    PeripheralDeviceStatus = osdp_sys.EVENT_NOTIFICATION_PD_STATUS

class Event:
    CardRead = osdp_sys.EVENT_CARDREAD
    KeyPress = osdp_sys.EVENT_KEYPRESS
    ManufacturerReply = osdp_sys.EVENT_MFGREP
    Status = osdp_sys.EVENT_STATUS
    Notification = osdp_sys.EVENT_NOTIFICATION

class CardFormat:
    Unspecified = osdp_sys.CARD_FMT_RAW_UNSPECIFIED
    Wiegand = osdp_sys.CARD_FMT_RAW_WIEGAND
    ASCII = osdp_sys.CARD_FMT_ASCII

class Capability:
    Unused = osdp_sys.CAP_UNUSED
    ContactStatusMonitoring = osdp_sys.CAP_CONTACT_STATUS_MONITORING
    OutputControl = osdp_sys.CAP_OUTPUT_CONTROL
    CardDataFormat = osdp_sys.CAP_CARD_DATA_FORMAT
    LEDControl = osdp_sys.CAP_READER_LED_CONTROL
    AudibleControl = osdp_sys.CAP_READER_AUDIBLE_OUTPUT
    TextOutput = osdp_sys.CAP_READER_TEXT_OUTPUT
    TimeKeeping = osdp_sys.CAP_TIME_KEEPING
    CheckCharacter = osdp_sys.CAP_CHECK_CHARACTER_SUPPORT
    CommunicationSecurity = osdp_sys.CAP_COMMUNICATION_SECURITY
    ReceiveBufferSize = osdp_sys.CAP_RECEIVE_BUFFERSIZE
    CombinedMessageSize = osdp_sys.CAP_LARGEST_COMBINED_MESSAGE_SIZE
    SmartCard = osdp_sys.CAP_SMART_CARD_SUPPORT
    Readers = osdp_sys.CAP_READERS
    Biometrics = osdp_sys.CAP_BIOMETRICS
