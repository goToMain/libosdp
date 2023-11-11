use crate::libosdp;

#[derive(Debug)]
pub enum OsdpCardFormats {
    Unspecified,
    Weigand,
    Ascii,
}

impl From<u32> for OsdpCardFormats {
    fn from(value: u32) -> Self {
        match value {
            libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED => {
                OsdpCardFormats::Unspecified
            }
            libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND => {
                OsdpCardFormats::Weigand
            }
            libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII => OsdpCardFormats::Ascii,
            _ => panic!("Unknown osdp card format"),
        }
    }
}

impl OsdpCardFormats {
    pub fn as_u32(&self) -> u32 {
        match self {
            OsdpCardFormats::Unspecified => {
                libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_UNSPECIFIED
            }
            OsdpCardFormats::Weigand => {
                libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_RAW_WIEGAND
            },
            OsdpCardFormats::Ascii => {
                libosdp::osdp_event_cardread_format_e_OSDP_CARD_FMT_ASCII
            },
        }
    }
}

#[derive(Debug)]
pub struct OsdpEventCardRead {
    reader_no: i32,
    format: OsdpCardFormats,
    direction: bool,
    data: [u8; 64],
}

impl From<libosdp::osdp_event_cardread> for OsdpEventCardRead {
    fn from(value: libosdp::osdp_event_cardread) -> Self {
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
    pub fn as_struct(&self) -> libosdp::osdp_event_cardread {
        libosdp::osdp_event_cardread {
            reader_no: self.reader_no,
            format: self.format.as_u32(),
            direction: self.direction as i32,
            length: self.data.len() as i32,
            data: self.data,
        }
    }
}

#[derive(Debug)]
pub struct OsdpEventKeyPress {
    reader_no: i32,
    data: [u8; 64],
}

impl From<libosdp::osdp_event_keypress> for OsdpEventKeyPress {
    fn from(value: libosdp::osdp_event_keypress) -> Self {
        OsdpEventKeyPress {
            reader_no: value.reader_no,
            data: value.data,
        }
    }
}

impl OsdpEventKeyPress {
    pub fn as_struct(&self) -> libosdp::osdp_event_keypress {
        libosdp::osdp_event_keypress {
            reader_no: self.reader_no,
            length: self.data.len() as i32,
            data: self.data,
        }
    }
}

#[derive(Debug)]
pub struct OsdpEventMfgReply {
    vendor_code: u32,
    command: u8,
    data: [u8; 64],
}

impl From<libosdp::osdp_event_mfgrep> for OsdpEventMfgReply {
    fn from(value: libosdp::osdp_event_mfgrep) -> Self {
        OsdpEventMfgReply {
            vendor_code: value.vendor_code,
            command: value.command,
            data: value.data,
        }
    }
}

impl OsdpEventMfgReply {
    pub fn as_struct(&self) -> libosdp::osdp_event_mfgrep {
        libosdp::osdp_event_mfgrep {
            vendor_code: self.vendor_code,
            command: self.command,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

#[derive(Debug)]
pub struct OsdpEventIO {
    type_: i32,
    status: u32,
}

impl From<libosdp::osdp_event_io> for OsdpEventIO {
    fn from(value: libosdp::osdp_event_io) -> Self {
        OsdpEventIO {
            type_: value.type_,
            status: value.status,
        }
    }
}

impl OsdpEventIO {
    pub fn as_struct(&self) -> libosdp::osdp_event_io {
        libosdp::osdp_event_io {
            type_: self.type_,
            status: self.status,
        }
    }
}

#[derive(Debug)]
pub struct OsdpEventStatus {
    tamper: u8,
    power: u8,
}

impl OsdpEventStatus {
    pub fn as_struct(&self) -> libosdp::osdp_event_status {
        libosdp::osdp_event_status {
            tamper: self.tamper,
            power: self.power,
        }
    }
}

impl From<libosdp::osdp_event_status> for OsdpEventStatus {
    fn from(value: libosdp::osdp_event_status) -> Self {
        OsdpEventStatus {
            tamper: value.tamper,
            power: value.power,
        }
    }
}

pub enum OsdpEvent {
    CardRead(OsdpEventCardRead),
    KeyPress(OsdpEventKeyPress),
    MfgReply(OsdpEventMfgReply),
    IO(OsdpEventIO),
    Status(OsdpEventStatus),
}

impl OsdpEvent {
    pub fn as_struct(&self) -> libosdp::osdp_event {
        match self {
            OsdpEvent::CardRead(e) => libosdp::osdp_event {
                type_: libosdp::osdp_event_type_OSDP_EVENT_CARDREAD,
                __bindgen_anon_1: libosdp::osdp_event__bindgen_ty_1 {
                    cardread: e.as_struct(),
                },
            },
            OsdpEvent::KeyPress(e) => libosdp::osdp_event {
                type_: libosdp::osdp_event_type_OSDP_EVENT_KEYPRESS,
                __bindgen_anon_1: libosdp::osdp_event__bindgen_ty_1 {
                    keypress: e.as_struct(),
                },
            },
            OsdpEvent::MfgReply(e) => libosdp::osdp_event {
                type_: libosdp::osdp_event_type_OSDP_EVENT_MFGREP,
                __bindgen_anon_1: libosdp::osdp_event__bindgen_ty_1 {
                    mfgrep: e.as_struct(),
                },
            },
            OsdpEvent::IO(e) => libosdp::osdp_event {
                type_: libosdp::osdp_event_type_OSDP_EVENT_IO,
                __bindgen_anon_1: libosdp::osdp_event__bindgen_ty_1 { io: e.as_struct() },
            },
            OsdpEvent::Status(e) => libosdp::osdp_event {
                type_: libosdp::osdp_event_type_OSDP_EVENT_STATUS,
                __bindgen_anon_1: libosdp::osdp_event__bindgen_ty_1 {
                    status: e.as_struct(),
                },
            },
        }
    }
}

impl From<libosdp::osdp_event> for OsdpEvent {
    fn from(value: libosdp::osdp_event) -> Self {
        match value.type_ {
            libosdp::osdp_event_type_OSDP_EVENT_CARDREAD => {
                OsdpEvent::CardRead(unsafe { value.__bindgen_anon_1.cardread.into() })
            }
            libosdp::osdp_event_type_OSDP_EVENT_KEYPRESS => {
                OsdpEvent::KeyPress(unsafe { value.__bindgen_anon_1.keypress.into() })
            }
            libosdp::osdp_event_type_OSDP_EVENT_MFGREP => {
                OsdpEvent::MfgReply(unsafe { value.__bindgen_anon_1.mfgrep.into() })
            }
            libosdp::osdp_event_type_OSDP_EVENT_IO => {
                OsdpEvent::IO(unsafe { value.__bindgen_anon_1.io.into() })
            }
            libosdp::osdp_event_type_OSDP_EVENT_STATUS => {
                OsdpEvent::Status(unsafe { value.__bindgen_anon_1.status.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}
