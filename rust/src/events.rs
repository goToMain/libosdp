//! OSDP PDs have to send messages to it's controlling unit - CP to intimate it
//! about various events that originate there (such as key press, card reads,
//! etc.,). They do this by creating an "event" and sending it to the CP. This
//! module is responsible to handling such events though [`OsdpEvent`].

use crate::{osdp_sys, ConvertEndian};
use serde::{Serialize, Deserialize};

/// Various card formats that a PD can support. This is sent to CP when a PD
/// must report a card read
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
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
            osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED => {
                OsdpCardFormats::Unspecified
            }
            osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND => {
                OsdpCardFormats::Weigand
            }
            osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII => OsdpCardFormats::Ascii,
            _ => panic!("Unknown osdp card format"),
        }
    }
}

impl Into<u32> for OsdpCardFormats {
    fn into(self) -> u32 {
        match self {
            OsdpCardFormats::Unspecified => {
                osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED
            }
            OsdpCardFormats::Weigand => {
                osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND
            },
            OsdpCardFormats::Ascii => {
                osdp_sys::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII
            },
        }
    }
}

/// Event that describes card read activity on the PD
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
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

    /// Card data; bytes or bits depending on [`OsdpCardFormats`]
    pub data: Vec<u8>,
}

impl OsdpEventCardRead {
    /// Create a card read event for self and direction set to forward (the most
    /// usual cases).
    pub fn new(format: OsdpCardFormats, data: Vec<u8>) -> Self {
        Self { reader_no: 0, format, direction: false, data }
    }
}

impl From<osdp_sys::osdp_event_cardread> for OsdpEventCardRead {
    fn from(value: osdp_sys::osdp_event_cardread) -> Self {
        let direction = if value.direction == 1 { true } else { false };
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        OsdpEventCardRead {
            reader_no: value.reader_no,
            format: value.format.into(),
            direction,
            data,
        }
    }
}

impl From<OsdpEventCardRead> for osdp_sys::osdp_event_cardread {
    fn from(value: OsdpEventCardRead) -> Self {
        let mut data: [u8; 64] = [0; 64];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        osdp_sys::osdp_event_cardread {
            reader_no: value.reader_no,
            format: value.format.clone().into(),
            direction: value.direction as i32,
            length: value.data.len() as i32,
            data,
        }
    }
}

/// Event to describe a key press activity on the PD
#[derive(Clone, Debug, Serialize, Deserialize)]
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

impl From<osdp_sys::osdp_event_keypress> for OsdpEventKeyPress {
    fn from(value: osdp_sys::osdp_event_keypress) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        OsdpEventKeyPress {
            reader_no: value.reader_no,
            data,
        }
    }
}

impl From<OsdpEventKeyPress> for osdp_sys::osdp_event_keypress {
    fn from(value: OsdpEventKeyPress) -> Self {
        let mut data: [u8; 64] = [0; 64];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        osdp_sys::osdp_event_keypress {
            reader_no: value.reader_no,
            length: value.data.len() as i32,
            data,
        }
    }
}

/// Event to transport a Manufacturer specific command's response.
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpEventMfgReply {
    /// 3-byte IEEE assigned OUI used as vendor code
    pub vendor_code: (u8, u8, u8),

    /// 1-byte reply code
    pub reply: u8,

    /// Reply data (if any)
    pub data: Vec<u8>,
}

impl From<osdp_sys::osdp_event_mfgrep> for OsdpEventMfgReply {
    fn from(value: osdp_sys::osdp_event_mfgrep) -> Self {
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

impl From<OsdpEventMfgReply> for osdp_sys::osdp_event_mfgrep {
    fn from(value: OsdpEventMfgReply) -> Self {
        let mut data: [u8; 64] = [0; 64];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        osdp_sys::osdp_event_mfgrep {
            vendor_code: value.vendor_code.as_le(),
            command: value.reply,
            length: value.data.len() as u8,
            data,
        }
    }
}

/// Event to describe change in input/output status on PD
///
/// This event is used by the PD to indicate input/output status changes. up to
/// a maximum of 32 input/output status can be reported. The values of the least
/// significant N bit of status are considered, where N is the number of items as
/// described in the corresponding capability codes,
/// - PdCapability::OutputControl
/// - PdCapability::ContactStatusMonitoring
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpEventIO {
    type_: i32,
    status: u32,
}

impl OsdpEventIO {
    /// Create an input event with given bit mask
    pub fn new_input(mask: u32) -> Self {
        Self { type_: 0, status: mask }
    }

    /// Create an output event with given bit mask
    pub fn new_output(mask: u32) -> Self {
        Self { type_: 0, status: mask }
    }
}

impl From<osdp_sys::osdp_event_io> for OsdpEventIO {
    fn from(value: osdp_sys::osdp_event_io) -> Self {
        OsdpEventIO {
            type_: value.type_,
            status: value.status,
        }
    }
}

impl From<OsdpEventIO> for osdp_sys::osdp_event_io {
    fn from(value: OsdpEventIO) -> Self {
        osdp_sys::osdp_event_io {
            type_: value.type_,
            status: value.status,
        }
    }
}

/// Event to describe a tamper/power status change
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpEventStatus {
    /// tamper status
    ///
    /// 0 - norma
    /// 1 - tamper
    pub tamper: u8,

    /// power status
    ///
    /// 0 - normal
    /// 1 - power failure
    pub power: u8,
}

impl From<OsdpEventStatus> for osdp_sys::osdp_event_status {
    fn from(value: OsdpEventStatus) -> Self {
        osdp_sys::osdp_event_status {
            tamper: value.tamper,
            power: value.power,
        }
    }
}

impl From<osdp_sys::osdp_event_status> for OsdpEventStatus {
    fn from(value: osdp_sys::osdp_event_status) -> Self {
        OsdpEventStatus {
            tamper: value.tamper,
            power: value.power,
        }
    }
}

/// CP to intimate it about various events that originate there (such as key
/// press, card reads, etc.,). They do this by creating an “event” and sending
/// it to the CP. This module is responsible to handling such events though
/// OsdpEvent.
#[derive(Clone, Debug, Serialize, Deserialize)]
pub enum OsdpEvent {
    /// Event that describes card read activity on the PD
    CardRead(OsdpEventCardRead),

    /// Event to describe a key press activity on the PD
    KeyPress(OsdpEventKeyPress),

    /// Event to transport a Manufacturer specific command’s response
    MfgReply(OsdpEventMfgReply),

    /// Event to describe change in input/output status on PD
    IO(OsdpEventIO),

    /// Event to describe a tamper/power status change
    Status(OsdpEventStatus),
}

impl From<OsdpEvent> for osdp_sys::osdp_event {
    fn from(value: OsdpEvent) -> Self {
        match value {
            OsdpEvent::CardRead(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_CARDREAD,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    cardread: e.clone().into(),
                },
            },
            OsdpEvent::KeyPress(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_KEYPRESS,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    keypress: e.clone().into(),
                },
            },
            OsdpEvent::MfgReply(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_MFGREP,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    mfgrep: e.clone().into(),
                },
            },
            OsdpEvent::IO(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_IO,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    io: e.clone().into()
                },
            },
            OsdpEvent::Status(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_STATUS,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    status: e.clone().into(),
                },
            },
        }
    }
}

impl From<osdp_sys::osdp_event> for OsdpEvent {
    fn from(value: osdp_sys::osdp_event) -> Self {
        match value.type_ {
            osdp_sys::osdp_event_type_OSDP_EVENT_CARDREAD => {
                OsdpEvent::CardRead(unsafe { value.__bindgen_anon_1.cardread.into() })
            }
            osdp_sys::osdp_event_type_OSDP_EVENT_KEYPRESS => {
                OsdpEvent::KeyPress(unsafe { value.__bindgen_anon_1.keypress.into() })
            }
            osdp_sys::osdp_event_type_OSDP_EVENT_MFGREP => {
                OsdpEvent::MfgReply(unsafe { value.__bindgen_anon_1.mfgrep.into() })
            }
            osdp_sys::osdp_event_type_OSDP_EVENT_IO => {
                OsdpEvent::IO(unsafe { value.__bindgen_anon_1.io.into() })
            }
            osdp_sys::osdp_event_type_OSDP_EVENT_STATUS => {
                OsdpEvent::Status(unsafe { value.__bindgen_anon_1.status.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}
