
pub struct OsdpLedParams {
    control_code: u8,
    on_count: u8,
    off_count: u8,
    on_color: u8,
    off_color: u8,
    timer_count: u16,
}

impl From <crate::osdp_cmd_led_params> for OsdpLedParams {
    fn from(value: crate::osdp_cmd_led_params) -> Self {
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
    pub fn as_struct(&self) -> crate::osdp_cmd_led_params {
        crate::osdp_cmd_led_params {
            control_code: self.control_code,
            on_count: self.on_count,
            off_count: self.off_count,
            on_color: self.on_color,
            off_color: self.off_color,
            timer_count: self.timer_count,
        }
    }
}

pub struct OsdpCommandLed {
    reader: u8,
    led_number: u8,
    temporary: OsdpLedParams,
    permanent: OsdpLedParams
}

impl From <crate::osdp_cmd_led> for OsdpCommandLed {
    fn from(value: crate::osdp_cmd_led) -> Self {
        OsdpCommandLed {
            reader: value.reader,
            led_number: value.reader,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

impl OsdpCommandLed {
    pub fn as_struct(&self) -> crate::osdp_cmd_led {
        crate::osdp_cmd_led {
            reader: self.reader,
            led_number: self.led_number,
            temporary: self.temporary.as_struct(),
            permanent: self.permanent.as_struct(),
        }
    }
}

pub struct OsdpCommandBuzzer {
    reader: u8,
    control_code: u8,
    on_count: u8,
    off_count: u8,
    rep_count: u8
}

impl From <crate::osdp_cmd_buzzer> for OsdpCommandBuzzer {
    fn from(value: crate::osdp_cmd_buzzer) -> Self {
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
    pub fn as_struct(&self) -> crate::osdp_cmd_buzzer {
        crate::osdp_cmd_buzzer {
            reader: self.reader,
            control_code: self.control_code,
            on_count: self.on_count,
            off_count: self.off_count,
            rep_count: self.rep_count,
        }
    }
}

pub struct OsdpCommandText {
    reader: u8, 
    control_code: u8, 
    temp_time: u8, 
    offset_row: u8, 
    offset_col: u8, 
    data: [u8; 32]
}

impl From <crate::osdp_cmd_text> for OsdpCommandText {
    fn from(value: crate::osdp_cmd_text) -> Self {
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
    pub fn as_struct(&self) -> crate::osdp_cmd_text {
        crate::osdp_cmd_text {
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

pub struct OsdpCommandOutput {
    output_no: u8,
    control_code: u8,
    timer_count: u16
}

impl From <crate::osdp_cmd_output> for OsdpCommandOutput {
    fn from(value: crate::osdp_cmd_output) -> Self {
        OsdpCommandOutput {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

impl OsdpCommandOutput {
    pub fn as_struct(&self) -> crate::osdp_cmd_output {
        crate::osdp_cmd_output {
            output_no: self.output_no,
            control_code: self.control_code,
            timer_count: self.timer_count,
        }
    }
}

pub struct OsdpComSet {
    address: u8,
    baud_rate: u32
}

impl From <crate::osdp_cmd_comset> for OsdpComSet {
    fn from(value: crate::osdp_cmd_comset) -> Self {
        OsdpComSet {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

impl OsdpComSet {
    pub fn as_struct(&self) -> crate::osdp_cmd_comset {
        crate::osdp_cmd_comset {
            address: self.address,
            baud_rate: self.baud_rate,
        }
    }
}

pub struct OsdpCommandKeyset {
    key_type: u8,
    data: [u8; 32]
}

impl From <crate::osdp_cmd_keyset> for OsdpCommandKeyset {
    fn from(value: crate::osdp_cmd_keyset) -> Self {
        OsdpCommandKeyset {
            key_type: value.type_,
            data: value.data,
        }
    }
}

impl OsdpCommandKeyset {
    pub fn as_struct(&self) -> crate::osdp_cmd_keyset {
        crate::osdp_cmd_keyset {
            type_: self.key_type,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

pub struct OsdpCommandMfg {
    vendor_code: u32,
    command: u8,
    data: [u8; 64]
}

impl From <crate::osdp_cmd_mfg> for OsdpCommandMfg {
    fn from(value: crate::osdp_cmd_mfg) -> Self {
        OsdpCommandMfg {
            vendor_code: value.vendor_code,
            command: value.command,
            data: value.data,
        }
    }
}

impl OsdpCommandMfg {
    pub fn as_struct(&self) -> crate::osdp_cmd_mfg {
        crate::osdp_cmd_mfg {
            vendor_code: self.vendor_code,
            command: self.command,
            length: self.data.len() as u8,
            data: self.data,
        }
    }
}

pub struct OsdpCommandFileTx {
    id: i32,
    flags: u32
}

impl From <crate::osdp_cmd_file_tx> for OsdpCommandFileTx {
    fn from(value: crate::osdp_cmd_file_tx) -> Self {
        OsdpCommandFileTx {
            id: value.id,
            flags: value.flags,
        }
    }
}

impl OsdpCommandFileTx {
    pub fn as_struct(&self) -> crate::osdp_cmd_file_tx {
        crate::osdp_cmd_file_tx {
            id: self.id,
            flags: self.flags,
        }
    }
}

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
    pub fn as_struct(&self) -> crate::osdp_cmd {
        match self {
            OsdpCommand::Led(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_LED,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    led: c.as_struct(),
                },
            },
            OsdpCommand::Buzzer(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_BUZZER,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    buzzer: c.as_struct()
                },
            },
            OsdpCommand::Text(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_TEXT,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    text: c.as_struct()
                },
            },
            OsdpCommand::Output(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_OUTPUT,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    output: c.as_struct()
                },
            },
            OsdpCommand::ComSet(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_COMSET,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    comset: c.as_struct()
                },
            },
            OsdpCommand::KeySet(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_KEYSET,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    keyset: c.as_struct()
                },
            },
            OsdpCommand::Mfg(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_MFG,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    mfg: c.as_struct()
                },
            },
            OsdpCommand::FileTx(c) => crate::osdp_cmd {
                id: crate::osdp_cmd_e_OSDP_CMD_FILE_TX,
                __bindgen_anon_1: crate::osdp_cmd__bindgen_ty_1 {
                    file_tx: c.as_struct()
                },
            },
        }
    }
}

impl From <crate::osdp_cmd> for OsdpCommand {
    fn from(value: crate::osdp_cmd) -> Self {
        match value.id {
            crate::osdp_cmd_e_OSDP_CMD_LED => {
                OsdpCommand::Led(unsafe { value.__bindgen_anon_1.led.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_BUZZER => {
                OsdpCommand::Buzzer(unsafe { value.__bindgen_anon_1.buzzer.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_TEXT => {
                OsdpCommand::Text(unsafe { value.__bindgen_anon_1.text.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_OUTPUT => {
                OsdpCommand::Output(unsafe { value.__bindgen_anon_1.output.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_COMSET => {
                OsdpCommand::ComSet(unsafe { value.__bindgen_anon_1.comset.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_KEYSET => {
                OsdpCommand::KeySet(unsafe { value.__bindgen_anon_1.keyset.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_MFG => {
                OsdpCommand::Mfg(unsafe { value.__bindgen_anon_1.mfg.into() })
            },
            crate::osdp_cmd_e_OSDP_CMD_FILE_TX => {
                OsdpCommand::FileTx(unsafe { value.__bindgen_anon_1.file_tx.into() })
            },
            _ => panic!("Unknown event"),
        }
    }
}