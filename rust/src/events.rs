use crate::osdp_sys;
use serde::{Serialize, Deserialize};
use serde_with::{serde_as, Bytes};

#[derive(Clone, Debug, Serialize, Deserialize)]
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

#[serde_as]
#[derive(Clone, Debug, Serialize, Deserialize)]
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

impl From<OsdpEventCardRead> for osdp_sys::osdp_event_cardread {
    fn from(value: OsdpEventCardRead) -> Self {
        osdp_sys::osdp_event_cardread {
            reader_no: value.reader_no,
            format: value.format.clone().into(),
            direction: value.direction as i32,
            length: value.data.len() as i32,
            data: value.data,
        }
    }
}

#[serde_as]
#[derive(Clone, Debug, Serialize, Deserialize)]
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

impl From<OsdpEventKeyPress> for osdp_sys::osdp_event_keypress {
    fn from(value: OsdpEventKeyPress) -> Self {
        osdp_sys::osdp_event_keypress {
            reader_no: value.reader_no,
            length: value.data.len() as i32,
            data: value.data,
        }
    }
}

#[serde_as]
#[derive(Clone, Debug, Serialize, Deserialize)]
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

impl From<OsdpEventMfgReply> for osdp_sys::osdp_event_mfgrep {
    fn from(value: OsdpEventMfgReply) -> Self {
        osdp_sys::osdp_event_mfgrep {
            vendor_code: value.vendor_code,
            command: value.command,
            length: value.data.len() as u8,
            data: value.data,
        }
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
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

impl From<OsdpEventIO> for osdp_sys::osdp_event_io {
    fn from(value: OsdpEventIO) -> Self {
        osdp_sys::osdp_event_io {
            type_: value.type_,
            status: value.status,
        }
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct OsdpEventStatus {
    tamper: u8,
    power: u8,
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

#[derive(Clone, Debug, Serialize, Deserialize)]
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
