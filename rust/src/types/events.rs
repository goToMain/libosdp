//! OSDP PDs have to send messages to it's controlling unit - CP to intimate it
//! about various events that originate there (such as key press, card reads,
//! etc.,). They do this by creating an "event" and sending it to the CP. This
//! module is responsible to handling such events though [`OsdpEvent`].

use crate::OsdpError;
use alloc::vec::Vec;
use serde::{Deserialize, Serialize};

use super::ConvertEndian;

type Result<T> = core::result::Result<T, OsdpError>;

/// Various card formats that a PD can support. This is sent to CP when a PD
/// must report a card read
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub enum OsdpCardFormats {
    /// Card format is not specified
    Unspecified,

    /// Weigand format
    Weigand,

    /// Ascii format
    #[default]
    Ascii,
}

impl From<u32> for OsdpCardFormats {
    fn from(value: u32) -> Self {
        match value {
            libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED => {
                OsdpCardFormats::Unspecified
            }
            libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND => {
                OsdpCardFormats::Weigand
            }
            libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII => OsdpCardFormats::Ascii,
            _ => panic!("Unknown osdp card format"),
        }
    }
}

impl Into<u32> for OsdpCardFormats {
    fn into(self) -> u32 {
        match self {
            OsdpCardFormats::Unspecified => {
                libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED
            }
            OsdpCardFormats::Weigand => {
                libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND
            }
            OsdpCardFormats::Ascii => libosdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII,
        }
    }
}

/// Event that describes card read activity on the PD
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpEventCardRead {
    /// Reader (another device connected to this PD) which caused this event
    ///
    /// 0 - self
    /// 1 - fist connected reader
    /// 2 - second connected reader
    /// ....
    pub reader_no: i32,

    /// Format of the card that was read
    pub format: OsdpCardFormats,

    /// The direction of the PD where the card read happened (some PDs have two
    /// physical card readers to put on either side of a door).
    ///
    /// false - Forward
    /// true - Backward
    pub direction: bool,

    /// Number of valid data bits in [`OsdpEventCardRead::data`] when the card
    /// format is [`OsdpCardFormats::Weigand`]. For all other formats, this
    /// field is set to zero.
    pub nr_bits: usize,

    /// Card data; bytes or bits depending on [`OsdpCardFormats`]
    pub data: Vec<u8>,
}

impl OsdpEventCardRead {
    /// Create an ASCII card read event for self and direction set to forward
    pub fn new_ascii(data: Vec<u8>) -> Self {
        Self {
            reader_no: 0,
            format: OsdpCardFormats::Ascii,
            direction: false,
            nr_bits: 0,
            data,
        }
    }

    /// Create a Weigand card read event for self and direction set to forward
    pub fn new_weigand(nr_bits: usize, data: Vec<u8>) -> Result<Self> {
        if nr_bits > data.len() * 8 {
            return Err(OsdpError::Command);
        }
        Ok(Self {
            reader_no: 0,
            format: OsdpCardFormats::Weigand,
            direction: false,
            nr_bits,
            data,
        })
    }
}

impl From<libosdp_sys::osdp_event_cardread> for OsdpEventCardRead {
    fn from(value: libosdp_sys::osdp_event_cardread) -> Self {
        let direction = if value.direction == 1 { true } else { false };
        let format = value.format.into();
        let len = value.length as usize;
        let (nr_bits, nr_bytes) = match format {
            OsdpCardFormats::Weigand => (len, (len + 7) / 8),
            _ => (0, len),
        };
        let data = value.data[0..nr_bytes].to_vec();
        OsdpEventCardRead {
            reader_no: value.reader_no,
            format,
            direction,
            nr_bits,
            data,
        }
    }
}

impl From<OsdpEventCardRead> for libosdp_sys::osdp_event_cardread {
    fn from(value: OsdpEventCardRead) -> Self {
        let mut data = [0; libosdp_sys::OSDP_EVENT_CARDREAD_MAX_DATALEN as usize];
        let length = match value.format {
            OsdpCardFormats::Weigand => value.nr_bits as i32,
            _ => value.data.len() as i32,
        };
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_event_cardread {
            reader_no: value.reader_no,
            format: value.format.clone().into(),
            direction: value.direction as i32,
            length,
            data,
        }
    }
}

/// Event to describe a key press activity on the PD
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpEventKeyPress {
    /// Reader (another device connected to this PD) which caused this event
    ///
    /// 0 - self
    /// 1 - fist connected reader
    /// 2 - second connected reader
    /// ....
    pub reader_no: i32,

    /// Key data
    pub data: Vec<u8>,
}

impl OsdpEventKeyPress {
    /// Create key press event for the keys specified in `data`.
    pub fn new(data: Vec<u8>) -> Self {
        Self { reader_no: 0, data }
    }
}

impl From<libosdp_sys::osdp_event_keypress> for OsdpEventKeyPress {
    fn from(value: libosdp_sys::osdp_event_keypress) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        OsdpEventKeyPress {
            reader_no: value.reader_no,
            data,
        }
    }
}

impl From<OsdpEventKeyPress> for libosdp_sys::osdp_event_keypress {
    fn from(value: OsdpEventKeyPress) -> Self {
        let mut data = [0; libosdp_sys::OSDP_EVENT_KEYPRESS_MAX_DATALEN as usize];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_event_keypress {
            reader_no: value.reader_no,
            length: value.data.len() as i32,
            data,
        }
    }
}

/// Event to transport a Manufacturer specific command's response.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpEventMfgReply {
    /// 3-byte IEEE assigned OUI used as vendor code
    pub vendor_code: (u8, u8, u8),

    /// 1-byte reply code
    pub reply: u8,

    /// Reply data (if any)
    pub data: Vec<u8>,
}

impl From<libosdp_sys::osdp_event_mfgrep> for OsdpEventMfgReply {
    fn from(value: libosdp_sys::osdp_event_mfgrep) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        let bytes = value.vendor_code.to_le_bytes();
        let vendor_code: (u8, u8, u8) = (bytes[0], bytes[1], bytes[2]);
        OsdpEventMfgReply {
            vendor_code,
            reply: value.command,
            data,
        }
    }
}

impl From<OsdpEventMfgReply> for libosdp_sys::osdp_event_mfgrep {
    fn from(value: OsdpEventMfgReply) -> Self {
        let mut data = [0; libosdp_sys::OSDP_EVENT_MFGREP_MAX_DATALEN as usize];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_event_mfgrep {
            vendor_code: value.vendor_code.as_le(),
            command: value.reply,
            length: value.data.len() as u8,
            data,
        }
    }
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
/// Status report type
pub enum OsdpStatusReportType {
    /// Input status report
    Input,
    /// Output status report
    Output,
    /// Remote status report
    Remote,
    /// Local status report
    Local,
}

impl From<u32> for OsdpStatusReportType {
    fn from(value: u32) -> Self {
        match value {
            libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_INPUT => {
                OsdpStatusReportType::Input
            }
            libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_OUTPUT => {
                OsdpStatusReportType::Output
            }
            libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_REMOTE => {
                OsdpStatusReportType::Remote
            }
            libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_LOCAL => {
                OsdpStatusReportType::Local
            }
            _ => panic!("Invalid enum entry"),
        }
    }
}

impl From<OsdpStatusReportType> for u32 {
    fn from(value: OsdpStatusReportType) -> Self {
        match value {
            OsdpStatusReportType::Input => {
                libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_INPUT as u32
            }
            OsdpStatusReportType::Output => {
                libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_OUTPUT as u32
            }
            OsdpStatusReportType::Remote => {
                libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_REMOTE as u32
            }
            OsdpStatusReportType::Local => {
                libosdp_sys::osdp_status_report_type_OSDP_STATUS_REPORT_LOCAL as u32
            }
        }
    }
}

/// Event to describe various status changes on PD
///
/// This event is used by the PD to indicate status such as input, output,
/// tamper, etc.,. up to a maximum of 32 status bits can be reported. The values
/// of the least significant N bit of status are considered, where N is the
/// number of items as described in the corresponding capability codes,
/// - PdCapability::OutputControl
/// - PdCapability::ContactStatusMonitoring
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpStatusReport {
    type_: OsdpStatusReportType,
    nr_entries: usize,
    mask: u32,
}

impl OsdpStatusReport {
    /// Create an input event with given bit mask
    pub fn new_input(nr_entries: usize, mask: u32) -> Self {
        Self {
            type_: OsdpStatusReportType::Input,
            nr_entries,
            mask,
        }
    }

    /// Create an output event with given bit mask
    pub fn new_output(nr_entries: usize, mask: u32) -> Self {
        Self {
            type_: OsdpStatusReportType::Output,
            nr_entries,
            mask,
        }
    }
}

impl From<libosdp_sys::osdp_status_report> for OsdpStatusReport {
    fn from(value: libosdp_sys::osdp_status_report) -> Self {
        OsdpStatusReport {
            type_: value.type_.into(),
            nr_entries: value.nr_entries as usize,
            mask: value.mask,
        }
    }
}

impl From<OsdpStatusReport> for libosdp_sys::osdp_status_report {
    fn from(value: OsdpStatusReport) -> Self {
        libosdp_sys::osdp_status_report {
            type_: value.type_.into(),
            nr_entries: value.nr_entries as i32,
            mask: value.mask,
        }
    }
}

/// CP to intimate it about various events that originate there (such as key
/// press, card reads, etc.,). They do this by creating an “event” and sending
/// it to the CP. This module is responsible to handling such events though
/// OsdpEvent.
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub enum OsdpEvent {
    /// Event that describes card read activity on the PD
    CardRead(OsdpEventCardRead),

    /// Event to describe a key press activity on the PD
    KeyPress(OsdpEventKeyPress),

    /// Event to transport a Manufacturer specific command’s response
    MfgReply(OsdpEventMfgReply),

    /// Event to describe a input/output/tamper/power status change
    Status(OsdpStatusReport),
}

impl From<OsdpEvent> for libosdp_sys::osdp_event {
    fn from(value: OsdpEvent) -> Self {
        match value {
            OsdpEvent::CardRead(e) => libosdp_sys::osdp_event {
                type_: libosdp_sys::osdp_event_type_OSDP_EVENT_CARDREAD,
                __bindgen_anon_1: libosdp_sys::osdp_event__bindgen_ty_1 {
                    cardread: e.clone().into(),
                },
            },
            OsdpEvent::KeyPress(e) => libosdp_sys::osdp_event {
                type_: libosdp_sys::osdp_event_type_OSDP_EVENT_KEYPRESS,
                __bindgen_anon_1: libosdp_sys::osdp_event__bindgen_ty_1 {
                    keypress: e.clone().into(),
                },
            },
            OsdpEvent::MfgReply(e) => libosdp_sys::osdp_event {
                type_: libosdp_sys::osdp_event_type_OSDP_EVENT_MFGREP,
                __bindgen_anon_1: libosdp_sys::osdp_event__bindgen_ty_1 {
                    mfgrep: e.clone().into(),
                },
            },
            OsdpEvent::Status(e) => libosdp_sys::osdp_event {
                type_: libosdp_sys::osdp_event_type_OSDP_EVENT_STATUS,
                __bindgen_anon_1: libosdp_sys::osdp_event__bindgen_ty_1 {
                    status: e.clone().into(),
                },
            },
        }
    }
}

impl From<libosdp_sys::osdp_event> for OsdpEvent {
    fn from(value: libosdp_sys::osdp_event) -> Self {
        match value.type_ {
            libosdp_sys::osdp_event_type_OSDP_EVENT_CARDREAD => {
                OsdpEvent::CardRead(unsafe { value.__bindgen_anon_1.cardread.into() })
            }
            libosdp_sys::osdp_event_type_OSDP_EVENT_KEYPRESS => {
                OsdpEvent::KeyPress(unsafe { value.__bindgen_anon_1.keypress.into() })
            }
            libosdp_sys::osdp_event_type_OSDP_EVENT_MFGREP => {
                OsdpEvent::MfgReply(unsafe { value.__bindgen_anon_1.mfgrep.into() })
            }
            libosdp_sys::osdp_event_type_OSDP_EVENT_STATUS => {
                OsdpEvent::Status(unsafe { value.__bindgen_anon_1.status.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::OsdpEventCardRead;
    use libosdp_sys::{
        osdp_event_cardread, osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII,
        osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND,
    };

    #[test]
    fn test_event_cardread() {
        let event = OsdpEventCardRead::new_ascii(vec![0x55, 0xAA]);
        let event_struct: osdp_event_cardread = event.clone().into();

        assert_eq!(event_struct.length, 2);
        assert_eq!(event_struct.direction, 0);
        assert_eq!(
            event_struct.format,
            osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII
        );
        assert_eq!(event_struct.data[0], 0x55);
        assert_eq!(event_struct.data[1], 0xAA);

        assert_eq!(event, event_struct.into());

        let event = OsdpEventCardRead::new_weigand(15, vec![0x55, 0xAA]).unwrap();
        let event_struct: osdp_event_cardread = event.clone().into();

        assert_eq!(event_struct.length, 15);
        assert_eq!(event_struct.direction, 0);
        assert_eq!(
            event_struct.format,
            osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND
        );
        assert_eq!(event_struct.data[0], 0x55);
        assert_eq!(event_struct.data[1], 0xAA);

        assert_eq!(event, event_struct.into());
    }
}
