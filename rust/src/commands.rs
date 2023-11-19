use crate::libosdp;
use serde::{Serialize, Deserialize};
use serde_with::{serde_as, Bytes};

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpLedParams {
    control_code: u8,
    on_count: u8,
    off_count: u8,
    on_color: u8,
    off_color: u8,
    timer_count: u16,
}

impl From<libosdp::osdp_cmd_led_params> for OsdpLedParams {
    fn from(value: libosdp::osdp_cmd_led_params) -> Self {
        OsdpLedParams {
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            on_color: value.on_color,
            off_color: value.off_color,
            timer_count: value.timer_count,
        }
    }
}

impl OsdpLedParams {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_led_params {
        libosdp::osdp_cmd_led_params {
            control_code: self.control_code,
            on_count: self.on_count,
            off_count: self.off_count,
            on_color: self.on_color,
            off_color: self.off_color,
            timer_count: self.timer_count,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandLed {
    reader: u8,
    led_number: u8,
    temporary: OsdpLedParams,
    permanent: OsdpLedParams,
}

impl From<libosdp::osdp_cmd_led> for OsdpCommandLed {
    fn from(value: libosdp::osdp_cmd_led) -> Self {
        OsdpCommandLed {
            reader: value.reader,
            led_number: value.reader,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

impl OsdpCommandLed {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_led {
        libosdp::osdp_cmd_led {
            reader: self.reader,
            led_number: self.led_number,
            temporary: self.temporary.as_struct(),
            permanent: self.permanent.as_struct(),
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandBuzzer {
    reader: u8,
    control_code: u8,
    on_count: u8,
    off_count: u8,
    rep_count: u8,
}

impl From<libosdp::osdp_cmd_buzzer> for OsdpCommandBuzzer {
    fn from(value: libosdp::osdp_cmd_buzzer) -> Self {
        OsdpCommandBuzzer {
            reader: value.reader,
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            rep_count: value.rep_count,
        }
    }
}

impl OsdpCommandBuzzer {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_buzzer {
        libosdp::osdp_cmd_buzzer {
            reader: self.reader,
            control_code: self.control_code,
            on_count: self.on_count,
            off_count: self.off_count,
            rep_count: self.rep_count,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandText {
    reader: u8,
    control_code: u8,
    temp_time: u8,
    offset_row: u8,
    offset_col: u8,
    data: [u8; 32],
}

impl From<libosdp::osdp_cmd_text> for OsdpCommandText {
    fn from(value: libosdp::osdp_cmd_text) -> Self {
        OsdpCommandText {
            reader: value.reader,
            control_code: value.control_code,
            temp_time: value.temp_time,
            offset_row: value.offset_row,
            offset_col: value.offset_col,
            data: value.data,
        }
    }
}

impl OsdpCommandText {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_text {
        libosdp::osdp_cmd_text {
            reader: self.reader,
            control_code: self.control_code,
            temp_time: self.temp_time,
            offset_row: self.offset_row,
            offset_col: self.offset_col,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandOutput {
    output_no: u8,
    control_code: u8,
    timer_count: u16,
}

impl From<libosdp::osdp_cmd_output> for OsdpCommandOutput {
    fn from(value: libosdp::osdp_cmd_output) -> Self {
        OsdpCommandOutput {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

impl OsdpCommandOutput {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_output {
        libosdp::osdp_cmd_output {
            output_no: self.output_no,
            control_code: self.control_code,
            timer_count: self.timer_count,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpComSet {
    address: u8,
    baud_rate: u32,
}

impl From<libosdp::osdp_cmd_comset> for OsdpComSet {
    fn from(value: libosdp::osdp_cmd_comset) -> Self {
        OsdpComSet {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

impl OsdpComSet {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_comset {
        libosdp::osdp_cmd_comset {
            address: self.address,
            baud_rate: self.baud_rate,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandKeyset {
    pub key_type: u8,
    pub data: [u8; 32],
}

impl From<libosdp::osdp_cmd_keyset> for OsdpCommandKeyset {
    fn from(value: libosdp::osdp_cmd_keyset) -> Self {
        OsdpCommandKeyset {
            key_type: value.type_,
            data: value.data,
        }
    }
}

impl OsdpCommandKeyset {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_keyset {
        libosdp::osdp_cmd_keyset {
            type_: self.key_type,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

#[serde_as]
#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandMfg {
    vendor_code: u32,
    command: u8,
    #[serde_as(as = "Bytes")]
    data: [u8; 64],
}

impl From<libosdp::osdp_cmd_mfg> for OsdpCommandMfg {
    fn from(value: libosdp::osdp_cmd_mfg) -> Self {
        OsdpCommandMfg {
            vendor_code: value.vendor_code,
            command: value.command,
            data: value.data,
        }
    }
}

impl OsdpCommandMfg {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_mfg {
        libosdp::osdp_cmd_mfg {
            vendor_code: self.vendor_code,
            command: self.command,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct OsdpCommandFileTx {
    id: i32,
    flags: u32,
}

impl From<libosdp::osdp_cmd_file_tx> for OsdpCommandFileTx {
    fn from(value: libosdp::osdp_cmd_file_tx) -> Self {
        OsdpCommandFileTx {
            id: value.id,
            flags: value.flags,
        }
    }
}

impl OsdpCommandFileTx {
    pub fn as_struct(&self) -> libosdp::osdp_cmd_file_tx {
        libosdp::osdp_cmd_file_tx {
            id: self.id,
            flags: self.flags,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub enum OsdpCommand {
    Led(OsdpCommandLed),
    Buzzer(OsdpCommandBuzzer),
    Text(OsdpCommandText),
    Output(OsdpCommandOutput),
    ComSet(OsdpComSet),
    KeySet(OsdpCommandKeyset),
    Mfg(OsdpCommandMfg),
    FileTx(OsdpCommandFileTx),
}

impl OsdpCommand {
    pub fn as_struct(&self) -> libosdp::osdp_cmd {
        match self {
            OsdpCommand::Led(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_LED,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 { led: c.as_struct() },
            },
            OsdpCommand::Buzzer(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_BUZZER,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    buzzer: c.as_struct(),
                },
            },
            OsdpCommand::Text(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_TEXT,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    text: c.as_struct(),
                },
            },
            OsdpCommand::Output(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_OUTPUT,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    output: c.as_struct(),
                },
            },
            OsdpCommand::ComSet(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_COMSET,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    comset: c.as_struct(),
                },
            },
            OsdpCommand::KeySet(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_KEYSET,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    keyset: c.as_struct(),
                },
            },
            OsdpCommand::Mfg(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_MFG,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 { mfg: c.as_struct() },
            },
            OsdpCommand::FileTx(c) => libosdp::osdp_cmd {
                id: libosdp::osdp_cmd_e_OSDP_CMD_FILE_TX,
                __bindgen_anon_1: libosdp::osdp_cmd__bindgen_ty_1 {
                    file_tx: c.as_struct(),
                },
            },
        }
    }
}

impl From<libosdp::osdp_cmd> for OsdpCommand {
    fn from(value: libosdp::osdp_cmd) -> Self {
        match value.id {
            libosdp::osdp_cmd_e_OSDP_CMD_LED => {
                OsdpCommand::Led(unsafe { value.__bindgen_anon_1.led.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_BUZZER => {
                OsdpCommand::Buzzer(unsafe { value.__bindgen_anon_1.buzzer.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_TEXT => {
                OsdpCommand::Text(unsafe { value.__bindgen_anon_1.text.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_OUTPUT => {
                OsdpCommand::Output(unsafe { value.__bindgen_anon_1.output.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_COMSET => {
                OsdpCommand::ComSet(unsafe { value.__bindgen_anon_1.comset.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_KEYSET => {
                OsdpCommand::KeySet(unsafe { value.__bindgen_anon_1.keyset.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_MFG => {
                OsdpCommand::Mfg(unsafe { value.__bindgen_anon_1.mfg.into() })
            }
            libosdp::osdp_cmd_e_OSDP_CMD_FILE_TX => {
                OsdpCommand::FileTx(unsafe { value.__bindgen_anon_1.file_tx.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}
