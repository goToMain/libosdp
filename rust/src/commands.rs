use crate::osdp_sys;
use serde::{Serialize, Deserialize};
use serde_with::{serde_as, Bytes};

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpLedParams {
    control_code: u8,
    on_count: u8,
    off_count: u8,
    on_color: u8,
    off_color: u8,
    timer_count: u16,
}

impl From<osdp_sys::osdp_cmd_led_params> for OsdpLedParams {
    fn from(value: osdp_sys::osdp_cmd_led_params) -> Self {
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

impl From<OsdpLedParams> for osdp_sys::osdp_cmd_led_params{
    fn from(value: OsdpLedParams) -> Self {
        osdp_sys::osdp_cmd_led_params {
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            on_color: value.on_color,
            off_color: value.off_color,
            timer_count: value.timer_count,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandLed {
    reader: u8,
    led_number: u8,
    temporary: OsdpLedParams,
    permanent: OsdpLedParams,
}

impl From<osdp_sys::osdp_cmd_led> for OsdpCommandLed {
    fn from(value: osdp_sys::osdp_cmd_led) -> Self {
        OsdpCommandLed {
            reader: value.reader,
            led_number: value.reader,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

impl From<OsdpCommandLed> for osdp_sys::osdp_cmd_led {
    fn from(value: OsdpCommandLed) -> Self {
        osdp_sys::osdp_cmd_led {
            reader: value.reader,
            led_number: value.led_number,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandBuzzer {
    reader: u8,
    control_code: u8,
    on_count: u8,
    off_count: u8,
    rep_count: u8,
}

impl From<osdp_sys::osdp_cmd_buzzer> for OsdpCommandBuzzer {
    fn from(value: osdp_sys::osdp_cmd_buzzer) -> Self {
        OsdpCommandBuzzer {
            reader: value.reader,
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            rep_count: value.rep_count,
        }
    }
}

impl From<OsdpCommandBuzzer> for osdp_sys::osdp_cmd_buzzer {
    fn from(value: OsdpCommandBuzzer) -> Self {
        osdp_sys::osdp_cmd_buzzer {
            reader: value.reader,
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            rep_count: value.rep_count,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandText {
    reader: u8,
    control_code: u8,
    temp_time: u8,
    offset_row: u8,
    offset_col: u8,
    data: [u8; 32],
}

impl From<osdp_sys::osdp_cmd_text> for OsdpCommandText {
    fn from(value: osdp_sys::osdp_cmd_text) -> Self {
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

impl From<OsdpCommandText> for osdp_sys::osdp_cmd_text {
    fn from(value: OsdpCommandText) -> Self {
        osdp_sys::osdp_cmd_text {
            reader: value.reader,
            control_code: value.control_code,
            temp_time: value.temp_time,
            offset_row: value.offset_row,
            offset_col: value.offset_col,
            length: value.data.len() as u8,
            data: value.data,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandOutput {
    output_no: u8,
    control_code: u8,
    timer_count: u16,
}

impl From<osdp_sys::osdp_cmd_output> for OsdpCommandOutput {
    fn from(value: osdp_sys::osdp_cmd_output) -> Self {
        OsdpCommandOutput {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

impl From<OsdpCommandOutput> for osdp_sys::osdp_cmd_output {
    fn from(value: OsdpCommandOutput) -> Self {
        osdp_sys::osdp_cmd_output {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpComSet {
    address: u8,
    baud_rate: u32,
}

impl From<osdp_sys::osdp_cmd_comset> for OsdpComSet {
    fn from(value: osdp_sys::osdp_cmd_comset) -> Self {
        OsdpComSet {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

impl From<OsdpComSet> for osdp_sys::osdp_cmd_comset {
    fn from(value: OsdpComSet) -> Self {
        osdp_sys::osdp_cmd_comset {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandKeyset {
    pub key_type: u8,
    pub data: [u8; 32],
}

impl From<osdp_sys::osdp_cmd_keyset> for OsdpCommandKeyset {
    fn from(value: osdp_sys::osdp_cmd_keyset) -> Self {
        OsdpCommandKeyset {
            key_type: value.type_,
            data: value.data,
        }
    }
}

impl From<OsdpCommandKeyset> for osdp_sys::osdp_cmd_keyset {
    fn from(value: OsdpCommandKeyset) -> Self {
        osdp_sys::osdp_cmd_keyset {
            type_: value.key_type,
            length: value.data.len() as u8,
            data: value.data,
        }
    }
}

#[serde_as]
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct OsdpCommandMfg {
    vendor_code: u32,
    command: u8,
    #[serde_as(as = "Bytes")]
    data: [u8; 64],
}

impl Default for OsdpCommandMfg {
    fn default() -> Self {
        Self {
            vendor_code: Default::default(),
            command: Default::default(),
            data: [0; 64]
        }
    }
}

impl From<osdp_sys::osdp_cmd_mfg> for OsdpCommandMfg {
    fn from(value: osdp_sys::osdp_cmd_mfg) -> Self {
        OsdpCommandMfg {
            vendor_code: value.vendor_code,
            command: value.command,
            data: value.data,
        }
    }
}

impl From<OsdpCommandMfg> for osdp_sys::osdp_cmd_mfg {
    fn from(value: OsdpCommandMfg) -> Self {
        osdp_sys::osdp_cmd_mfg {
            vendor_code: value.vendor_code,
            command: value.command,
            length: value.data.len() as u8,
            data: value.data,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct OsdpCommandFileTx {
    id: i32,
    flags: u32,
}

impl From<osdp_sys::osdp_cmd_file_tx> for OsdpCommandFileTx {
    fn from(value: osdp_sys::osdp_cmd_file_tx) -> Self {
        OsdpCommandFileTx {
            id: value.id,
            flags: value.flags,
        }
    }
}

impl From<OsdpCommandFileTx> for osdp_sys::osdp_cmd_file_tx {
    fn from(value: OsdpCommandFileTx) -> Self {
        osdp_sys::osdp_cmd_file_tx {
            id: value.id,
            flags: value.flags,
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

impl From<OsdpCommand> for osdp_sys::osdp_cmd {
    fn from(value: OsdpCommand) -> Self {
        match value {
            OsdpCommand::Led(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_LED,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    led: c.clone().into()
                },
            },
            OsdpCommand::Buzzer(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_BUZZER,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    buzzer: c.clone().into(),
                },
            },
            OsdpCommand::Text(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_TEXT,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    text: c.clone().into(),
                },
            },
            OsdpCommand::Output(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_OUTPUT,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    output: c.clone().into(),
                },
            },
            OsdpCommand::ComSet(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_COMSET,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    comset: c.clone().into(),
                },
            },
            OsdpCommand::KeySet(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_KEYSET,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    keyset: c.clone().into(),
                },
            },
            OsdpCommand::Mfg(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_MFG,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    mfg: c.clone().into()
                },
            },
            OsdpCommand::FileTx(c) => osdp_sys::osdp_cmd {
                id: osdp_sys::osdp_cmd_e_OSDP_CMD_FILE_TX,
                __bindgen_anon_1: osdp_sys::osdp_cmd__bindgen_ty_1 {
                    file_tx: c.clone().into(),
                },
            },
        }
    }
}

impl From<osdp_sys::osdp_cmd> for OsdpCommand {
    fn from(value: osdp_sys::osdp_cmd) -> Self {
        match value.id {
            osdp_sys::osdp_cmd_e_OSDP_CMD_LED => {
                OsdpCommand::Led(unsafe { value.__bindgen_anon_1.led.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_BUZZER => {
                OsdpCommand::Buzzer(unsafe { value.__bindgen_anon_1.buzzer.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_TEXT => {
                OsdpCommand::Text(unsafe { value.__bindgen_anon_1.text.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_OUTPUT => {
                OsdpCommand::Output(unsafe { value.__bindgen_anon_1.output.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_COMSET => {
                OsdpCommand::ComSet(unsafe { value.__bindgen_anon_1.comset.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_KEYSET => {
                OsdpCommand::KeySet(unsafe { value.__bindgen_anon_1.keyset.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_MFG => {
                OsdpCommand::Mfg(unsafe { value.__bindgen_anon_1.mfg.into() })
            }
            osdp_sys::osdp_cmd_e_OSDP_CMD_FILE_TX => {
                OsdpCommand::FileTx(unsafe { value.__bindgen_anon_1.file_tx.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}
