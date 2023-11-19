use crate::osdp_sys;
use serde::{Serialize, Deserialize};
use serde_with::{serde_as, Bytes};

#[derive(Debug, Serialize, Deserialize)]
pub enum OsdpCardFormats {
    Unspecified,
    Weigand,
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

impl OsdpCardFormats {
    pub fn as_u32(&self) -> u32 {
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

#[serde_as]
#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpEventCardRead {
    reader_no: i32,
    format: OsdpCardFormats,
    direction: bool,
    #[serde_as(as = "Bytes")]
    data: [u8; 64],
}

impl From<osdp_sys::osdp_event_cardread> for OsdpEventCardRead {
    fn from(value: osdp_sys::osdp_event_cardread) -> Self {
        let direction = if value.direction == 1 { true } else { false };
        OsdpEventCardRead {
            reader_no: value.reader_no,
            format: value.format.into(),
            direction,
            data: value.data,
        }
    }
}

impl OsdpEventCardRead {
    pub fn as_struct(&self) -> osdp_sys::osdp_event_cardread {
        osdp_sys::osdp_event_cardread {
            reader_no: self.reader_no,
            format: self.format.as_u32(),
            direction: self.direction as i32,
            length: self.data.len() as i32,
            data: self.data,
        }
    }
}

#[serde_as]
#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpEventKeyPress {
    reader_no: i32,
    #[serde_as(as = "Bytes")]
    data: [u8; 64],
}

impl From<osdp_sys::osdp_event_keypress> for OsdpEventKeyPress {
    fn from(value: osdp_sys::osdp_event_keypress) -> Self {
        OsdpEventKeyPress {
            reader_no: value.reader_no,
            data: value.data,
        }
    }
}

impl OsdpEventKeyPress {
    pub fn as_struct(&self) -> osdp_sys::osdp_event_keypress {
        osdp_sys::osdp_event_keypress {
            reader_no: self.reader_no,
            length: self.data.len() as i32,
            data: self.data,
        }
    }
}

#[serde_as]
#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpEventMfgReply {
    vendor_code: u32,
    command: u8,
    #[serde_as(as = "Bytes")]
    data: [u8; 64],
}

impl From<osdp_sys::osdp_event_mfgrep> for OsdpEventMfgReply {
    fn from(value: osdp_sys::osdp_event_mfgrep) -> Self {
        OsdpEventMfgReply {
            vendor_code: value.vendor_code,
            command: value.command,
            data: value.data,
        }
    }
}

impl OsdpEventMfgReply {
    pub fn as_struct(&self) -> osdp_sys::osdp_event_mfgrep {
        osdp_sys::osdp_event_mfgrep {
            vendor_code: self.vendor_code,
            command: self.command,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpEventIO {
    type_: i32,
    status: u32,
}

impl From<osdp_sys::osdp_event_io> for OsdpEventIO {
    fn from(value: osdp_sys::osdp_event_io) -> Self {
        OsdpEventIO {
            type_: value.type_,
            status: value.status,
        }
    }
}

impl OsdpEventIO {
    pub fn as_struct(&self) -> osdp_sys::osdp_event_io {
        osdp_sys::osdp_event_io {
            type_: self.type_,
            status: self.status,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpEventStatus {
    tamper: u8,
    power: u8,
}

impl OsdpEventStatus {
    pub fn as_struct(&self) -> osdp_sys::osdp_event_status {
        osdp_sys::osdp_event_status {
            tamper: self.tamper,
            power: self.power,
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

#[derive(Debug, Serialize, Deserialize)]
pub enum OsdpEvent {
    CardRead(OsdpEventCardRead),
    KeyPress(OsdpEventKeyPress),
    MfgReply(OsdpEventMfgReply),
    IO(OsdpEventIO),
    Status(OsdpEventStatus),
}

impl OsdpEvent {
    pub fn as_struct(&self) -> osdp_sys::osdp_event {
        match self {
            OsdpEvent::CardRead(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_CARDREAD,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    cardread: e.as_struct(),
                },
            },
            OsdpEvent::KeyPress(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_KEYPRESS,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    keypress: e.as_struct(),
                },
            },
            OsdpEvent::MfgReply(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_MFGREP,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    mfgrep: e.as_struct(),
                },
            },
            OsdpEvent::IO(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_IO,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 { io: e.as_struct() },
            },
            OsdpEvent::Status(e) => osdp_sys::osdp_event {
                type_: osdp_sys::osdp_event_type_OSDP_EVENT_STATUS,
                __bindgen_anon_1: osdp_sys::osdp_event__bindgen_ty_1 {
                    status: e.as_struct(),
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
