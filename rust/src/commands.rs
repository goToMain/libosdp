//! CP interacts with and controls PDs by sending commands to it. These commands
//! are specified by OSDP specification. This module is responsible to handling
//! such commands though [`OsdpCommand`].

use crate::ConvertEndian;
use alloc::vec::Vec;
use serde::{Deserialize, Serialize};

/// LED Colors as specified in OSDP for the on_color/off_color parameters.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub enum OsdpLedColor {
    /// No Color
    #[default]
    None,

    /// Red Color
    Red,

    /// Green Color
    Green,

    /// Amber Color
    Amber,

    /// Blue Color
    Blue,

    /// Magenta Color
    Magenta,

    /// Cyan Color
    Cyan,
}

impl From<u8> for OsdpLedColor {
    fn from(value: u8) -> Self {
        match value as u32 {
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_NONE => OsdpLedColor::None,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_RED => OsdpLedColor::Red,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_GREEN => OsdpLedColor::Green,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_AMBER => OsdpLedColor::Amber,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_BLUE => OsdpLedColor::Blue,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_MAGENTA => OsdpLedColor::Magenta,
            libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_CYAN => OsdpLedColor::Cyan,
            _ => panic!("Invalid LED color code"),
        }
    }
}

impl From<OsdpLedColor> for u8 {
    fn from(value: OsdpLedColor) -> Self {
        match value {
            OsdpLedColor::None => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_NONE as u8,
            OsdpLedColor::Red => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_RED as u8,
            OsdpLedColor::Green => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_GREEN as u8,
            OsdpLedColor::Amber => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_AMBER as u8,
            OsdpLedColor::Blue => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_BLUE as u8,
            OsdpLedColor::Magenta => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_MAGENTA as u8,
            OsdpLedColor::Cyan => libosdp_sys::osdp_led_color_e_OSDP_LED_COLOR_CYAN as u8,
        }
    }
}

/// LED params sub-structure. Part of LED command: OsdpCommandLed
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpLedParams {
    /// Control code serves different purposes based on which member of
    /// [`OsdpCommandLed`] it is used with. They are,
    ///
    /// temporary:
    ///
    /// 0 - NOP - do not alter this LED's temporary settings
    /// 1 - Cancel any temporary operation and display this LED's permanent state immediately
    /// 2 - Set the temporary state as given and start timer immediately
    ///
    /// permanent:
    ///
    /// 0 - NOP - do not alter this LED's permanent settings
    /// 1 - Set the permanent state as given
    pub control_code: u8,

    /// The ON duration of the flash, in units of 100 ms
    pub on_count: u8,

    /// The OFF duration of the flash, in units of 100 ms
    pub off_count: u8,

    /// Color to set during the ON timer
    pub on_color: OsdpLedColor,

    /// Color to set during the Off timer
    pub off_color: OsdpLedColor,

    /// Time in units of 100 ms (only for temporary mode)
    pub timer_count: u16,
}

impl From<libosdp_sys::osdp_cmd_led_params> for OsdpLedParams {
    fn from(value: libosdp_sys::osdp_cmd_led_params) -> Self {
        OsdpLedParams {
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            on_color: value.on_color.into(),
            off_color: value.off_color.into(),
            timer_count: value.timer_count,
        }
    }
}

impl From<OsdpLedParams> for libosdp_sys::osdp_cmd_led_params {
    fn from(value: OsdpLedParams) -> Self {
        libosdp_sys::osdp_cmd_led_params {
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            on_color: value.on_color.into(),
            off_color: value.off_color.into(),
            timer_count: value.timer_count,
        }
    }
}

/// Command to control the behavior of it's on-board LEDs
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandLed {
    /// Reader (another device connected to this PD) for which this command is
    /// issued for.
    ///
    /// 0 - self
    /// 1 - fist connected reader
    /// 2 - second connected reader
    /// ....
    pub reader: u8,

    /// LED number to operate on; 0 = first LED, 1 = second LED, etc.
    pub led_number: u8,

    /// Temporary LED activity descriptor. This operation is ephemeral and
    /// interrupts any on going permanent activity.
    pub temporary: OsdpLedParams,

    /// Permanent LED activity descriptor. This operation continues till another
    /// permanent activity overwrites this state.
    pub permanent: OsdpLedParams,
}

impl From<libosdp_sys::osdp_cmd_led> for OsdpCommandLed {
    fn from(value: libosdp_sys::osdp_cmd_led) -> Self {
        OsdpCommandLed {
            reader: value.reader,
            led_number: value.reader,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

impl From<OsdpCommandLed> for libosdp_sys::osdp_cmd_led {
    fn from(value: OsdpCommandLed) -> Self {
        libosdp_sys::osdp_cmd_led {
            reader: value.reader,
            led_number: value.led_number,
            temporary: value.temporary.into(),
            permanent: value.permanent.into(),
        }
    }
}

/// Command to control the behavior of a buzzer in the PD
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandBuzzer {
    /// Reader (another device connected to this PD) for which this command is
    /// issued for.
    ///
    /// 0 - self
    /// 1 - fist connected reader
    /// 2 - second connected reader
    /// ....
    pub reader: u8,

    /// Control code instructs the operation to perform:
    ///
    /// 0 - no tone
    /// 1 - off
    /// 2 - default tone
    /// 3+ - TBD
    pub control_code: u8,

    /// The ON duration of the flash, in units of 100 ms
    pub on_count: u8,

    /// The OFF duration of the flash, in units of 100 ms
    pub off_count: u8,

    /// The number of times to repeat the ON/OFF cycle; Setting this value to 0
    /// indicates the action is to be repeated forever.
    pub rep_count: u8,
}

impl From<libosdp_sys::osdp_cmd_buzzer> for OsdpCommandBuzzer {
    fn from(value: libosdp_sys::osdp_cmd_buzzer) -> Self {
        OsdpCommandBuzzer {
            reader: value.reader,
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            rep_count: value.rep_count,
        }
    }
}

impl From<OsdpCommandBuzzer> for libosdp_sys::osdp_cmd_buzzer {
    fn from(value: OsdpCommandBuzzer) -> Self {
        libosdp_sys::osdp_cmd_buzzer {
            reader: value.reader,
            control_code: value.control_code,
            on_count: value.on_count,
            off_count: value.off_count,
            rep_count: value.rep_count,
        }
    }
}

/// Command to manipulate the on-board display unit (Can be LED, LCD, 7-Segment,
/// etc.,) on the PD.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandText {
    /// Reader (another device connected to this PD) for which this command is
    /// issued for.
    ///
    /// 0 - self
    /// 1 - fist connected reader
    /// 2 - second connected reader
    /// ....
    pub reader: u8,

    /// Control code instructs the operation to perform:
    ///
    /// 1 - permanent text, no wrap
    /// 2 - permanent text, with wrap
    /// 3 - temporary text, no wrap
    /// 4 - temporary text, with wrap
    pub control_code: u8,

    /// duration to display temporary text, in seconds
    pub temp_time: u8,

    /// row to display the first character (1 indexed)
    pub offset_row: u8,

    /// column to display the first character (1 indexed)
    pub offset_col: u8,

    /// The string to display (ASCII codes)
    pub data: Vec<u8>,
}

impl From<libosdp_sys::osdp_cmd_text> for OsdpCommandText {
    fn from(value: libosdp_sys::osdp_cmd_text) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        OsdpCommandText {
            reader: value.reader,
            control_code: value.control_code,
            temp_time: value.temp_time,
            offset_row: value.offset_row,
            offset_col: value.offset_col,
            data,
        }
    }
}

impl From<OsdpCommandText> for libosdp_sys::osdp_cmd_text {
    fn from(value: OsdpCommandText) -> Self {
        let mut data = [0; libosdp_sys::OSDP_CMD_TEXT_MAX_LEN as usize];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_cmd_text {
            reader: value.reader,
            control_code: value.control_code,
            temp_time: value.temp_time,
            offset_row: value.offset_row,
            offset_col: value.offset_col,
            length: value.data.len() as u8,
            data,
        }
    }
}

/// Command to control digital output exposed by the PD.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandOutput {
    /// The output number this to apply this action.
    ///
    /// 0 - First Output
    /// 1 - Second Output
    /// ....
    pub output_no: u8,

    /// Control code instructs the operation to perform:
    ///
    /// 0 - NOP – do not alter this output
    /// 1 - set the permanent state to OFF, abort timed operation (if any)
    /// 2 - set the permanent state to ON, abort timed operation (if any)
    /// 3 - set the permanent state to OFF, allow timed operation to complete
    /// 4 - set the permanent state to ON, allow timed operation to complete
    /// 5 - set the temporary state to ON, resume perm state on timeout
    /// 6 - set the temporary state to OFF, resume permanent state on timeout
    pub control_code: u8,

    ///  Time in units of 100 ms
    pub timer_count: u16,
}

impl From<libosdp_sys::osdp_cmd_output> for OsdpCommandOutput {
    fn from(value: libosdp_sys::osdp_cmd_output) -> Self {
        OsdpCommandOutput {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

impl From<OsdpCommandOutput> for libosdp_sys::osdp_cmd_output {
    fn from(value: OsdpCommandOutput) -> Self {
        libosdp_sys::osdp_cmd_output {
            output_no: value.output_no,
            control_code: value.control_code,
            timer_count: value.timer_count,
        }
    }
}

/// Command to set the communication parameters for the PD. The effects of this
/// command is expected to be be stored in PD's non-volatile memory as the CP
/// will expect the PD to be in this state moving forward.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpComSet {
    address: u8,
    baud_rate: u32,
}

impl OsdpComSet {
    /// Create an instance of OsdpComSet command
    ///
    /// # Arguments
    ///
    /// * `address` - address to which this PD will respond after this command
    /// * `baud_rate` - Serial communication speed; only acceptable values are,
    ///   9600/19200/38400/57600/115200/230400
    pub fn new(address: u8, baud_rate: u32) -> Self {
        Self { address, baud_rate }
    }
}

impl From<libosdp_sys::osdp_cmd_comset> for OsdpComSet {
    fn from(value: libosdp_sys::osdp_cmd_comset) -> Self {
        OsdpComSet {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

impl From<OsdpComSet> for libosdp_sys::osdp_cmd_comset {
    fn from(value: OsdpComSet) -> Self {
        libosdp_sys::osdp_cmd_comset {
            address: value.address,
            baud_rate: value.baud_rate,
        }
    }
}

/// Command to set secure channel keys to the PD.
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandKeyset {
    key_type: u8,
    /// Key data
    pub data: Vec<u8>,
}

impl OsdpCommandKeyset {
    /// Create a new SCBK KeySet command for a given key
    ///
    /// # Arguments
    ///
    /// * `key` - 16 bytes of secure channel base key
    pub fn new_scbk(key: [u8; 16]) -> Self {
        let data = key.to_vec();
        Self { key_type: 1, data }
    }
}

impl From<libosdp_sys::osdp_cmd_keyset> for OsdpCommandKeyset {
    fn from(value: libosdp_sys::osdp_cmd_keyset) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        OsdpCommandKeyset {
            key_type: value.type_,
            data,
        }
    }
}

impl From<OsdpCommandKeyset> for libosdp_sys::osdp_cmd_keyset {
    fn from(value: OsdpCommandKeyset) -> Self {
        let mut data = [0; libosdp_sys::OSDP_CMD_KEYSET_KEY_MAX_LEN as usize];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_cmd_keyset {
            type_: value.key_type,
            length: value.data.len() as u8,
            data,
        }
    }
}

/// Command to to act as a wrapper for manufacturer specific commands
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandMfg {
    /// 3-byte IEEE assigned OUI used as vendor code
    pub vendor_code: (u8, u8, u8),

    /// 1-byte manufacturer defined command ID
    pub command: u8,

    /// Command data (if any)
    pub data: Vec<u8>,
}

impl From<libosdp_sys::osdp_cmd_mfg> for OsdpCommandMfg {
    fn from(value: libosdp_sys::osdp_cmd_mfg) -> Self {
        let n = value.length as usize;
        let data = value.data[0..n].to_vec();
        let bytes = value.vendor_code.to_le_bytes();
        let vendor_code: (u8, u8, u8) = (bytes[0], bytes[1], bytes[2]);
        OsdpCommandMfg {
            vendor_code,
            command: value.command,
            data,
        }
    }
}

impl From<OsdpCommandMfg> for libosdp_sys::osdp_cmd_mfg {
    fn from(value: OsdpCommandMfg) -> Self {
        let mut data = [0; libosdp_sys::OSDP_CMD_MFG_MAX_DATALEN as usize];
        for i in 0..value.data.len() {
            data[i] = value.data[i];
        }
        libosdp_sys::osdp_cmd_mfg {
            vendor_code: value.vendor_code.as_le(),
            command: value.command,
            length: value.data.len() as u8,
            data,
        }
    }
}

/// Command to kick-off a file transfer to the PD.
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct OsdpCommandFileTx {
    id: i32,
    flags: u32,
}

impl OsdpCommandFileTx {
    /// Create an instance of OsdpCommandFileTx.
    ///
    /// # Arguments
    ///
    /// * `id` - The ID of the file; these are pre-shared between the CP and PD
    /// * `flags` - Reserved and set to zero by OSDP spec; bit-31 used by
    ///   libOSDP to cancel ongoing transfers (it is not sent on OSDP channel)
    pub fn new(id: i32, flags: u32) -> Self {
        Self { id, flags }
    }
}

impl From<libosdp_sys::osdp_cmd_file_tx> for OsdpCommandFileTx {
    fn from(value: libosdp_sys::osdp_cmd_file_tx) -> Self {
        OsdpCommandFileTx {
            id: value.id,
            flags: value.flags,
        }
    }
}

impl From<OsdpCommandFileTx> for libosdp_sys::osdp_cmd_file_tx {
    fn from(value: OsdpCommandFileTx) -> Self {
        libosdp_sys::osdp_cmd_file_tx {
            id: value.id,
            flags: value.flags,
        }
    }
}

/// Command to query status from PD
#[derive(Clone, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub enum OsdpCommandStatus {
    /// Query local status (tamper and power) from PD
    #[default]
    Local,
    /// Query input status from PD
    Input,
    /// Query output status from PD
    Output,
}

impl From<libosdp_sys::osdp_cmd_status> for OsdpCommandStatus {
    fn from(value: libosdp_sys::osdp_cmd_status) -> Self {
        match value.type_ {
            libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_INPUT => {
                OsdpCommandStatus::Input
            }
            libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_OUTPUT => {
                OsdpCommandStatus::Output
            }
            libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_LOCAL => {
                OsdpCommandStatus::Local
            }
            _ => panic!("Unknown status command type"),
        }
    }
}

impl From<OsdpCommandStatus> for libosdp_sys::osdp_cmd_status {
    fn from(value: OsdpCommandStatus) -> Self {
        match value {
            OsdpCommandStatus::Input => libosdp_sys::osdp_cmd_status {
                type_: libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_INPUT,
            },
            OsdpCommandStatus::Output => libosdp_sys::osdp_cmd_status {
                type_: libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_OUTPUT,
            },
            OsdpCommandStatus::Local => libosdp_sys::osdp_cmd_status {
                type_: libosdp_sys::osdp_command_status_query_e_OSDP_CMD_STATUS_QUERY_LOCAL,
            },
        }
    }
}

/// CP interacts with and controls PDs by sending commands to it. The commands
/// in this enum are specified by OSDP specification.
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub enum OsdpCommand {
    /// Command to control the behavior of it’s on-board LEDs
    Led(OsdpCommandLed),

    /// Command to control the behavior of a buzzer in the PD
    Buzzer(OsdpCommandBuzzer),

    /// Command to manipulate the on-board display unit (Can be LED, LCD,
    /// 7-Segment, etc.,) on the PD
    Text(OsdpCommandText),

    /// Command to control digital output exposed by the PD
    Output(OsdpCommandOutput),

    /// Command to set the communication parameters for the PD. The effects
    /// of this command is expected to be be stored in PD’s non-volatile memory
    /// as the CP will expect the PD to be in this state moving forward
    ComSet(OsdpComSet),

    /// Command to set secure channel keys to the PD
    KeySet(OsdpCommandKeyset),

    /// Command to to act as a wrapper for manufacturer specific commands
    Mfg(OsdpCommandMfg),

    /// Command to kick-off a file transfer to the PD
    FileTx(OsdpCommandFileTx),

    /// Command to query status from the PD
    Status(OsdpCommandStatus),
}

impl From<OsdpCommand> for libosdp_sys::osdp_cmd {
    fn from(value: OsdpCommand) -> Self {
        match value {
            OsdpCommand::Led(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_LED,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    led: c.clone().into(),
                },
            },
            OsdpCommand::Buzzer(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_BUZZER,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    buzzer: c.clone().into(),
                },
            },
            OsdpCommand::Text(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_TEXT,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    text: c.clone().into(),
                },
            },
            OsdpCommand::Output(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_OUTPUT,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    output: c.clone().into(),
                },
            },
            OsdpCommand::ComSet(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_COMSET,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    comset: c.clone().into(),
                },
            },
            OsdpCommand::KeySet(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_KEYSET,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    keyset: c.clone().into(),
                },
            },
            OsdpCommand::Mfg(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_MFG,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    mfg: c.clone().into(),
                },
            },
            OsdpCommand::FileTx(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_FILE_TX,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    file_tx: c.clone().into(),
                },
            },
            OsdpCommand::Status(c) => libosdp_sys::osdp_cmd {
                id: libosdp_sys::osdp_cmd_e_OSDP_CMD_STATUS,
                __bindgen_anon_1: libosdp_sys::osdp_cmd__bindgen_ty_1 {
                    status: c.clone().into(),
                },
            },
        }
    }
}

impl From<libosdp_sys::osdp_cmd> for OsdpCommand {
    fn from(value: libosdp_sys::osdp_cmd) -> Self {
        match value.id {
            libosdp_sys::osdp_cmd_e_OSDP_CMD_LED => {
                OsdpCommand::Led(unsafe { value.__bindgen_anon_1.led.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_BUZZER => {
                OsdpCommand::Buzzer(unsafe { value.__bindgen_anon_1.buzzer.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_TEXT => {
                OsdpCommand::Text(unsafe { value.__bindgen_anon_1.text.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_OUTPUT => {
                OsdpCommand::Output(unsafe { value.__bindgen_anon_1.output.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_COMSET => {
                OsdpCommand::ComSet(unsafe { value.__bindgen_anon_1.comset.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_KEYSET => {
                OsdpCommand::KeySet(unsafe { value.__bindgen_anon_1.keyset.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_MFG => {
                OsdpCommand::Mfg(unsafe { value.__bindgen_anon_1.mfg.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_FILE_TX => {
                OsdpCommand::FileTx(unsafe { value.__bindgen_anon_1.file_tx.into() })
            }
            libosdp_sys::osdp_cmd_e_OSDP_CMD_STATUS => {
                OsdpCommand::Status(unsafe { value.__bindgen_anon_1.status.into() })
            }
            _ => panic!("Unknown event"),
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::commands::OsdpCommandMfg;
    use libosdp_sys::osdp_cmd_mfg;

    #[test]
    fn test_command_mfg() {
        let cmd = OsdpCommandMfg {
            vendor_code: (0x05, 0x07, 0x09),
            command: 0x47,
            data: vec![0x55, 0xAA],
        };
        let cmd_struct: osdp_cmd_mfg = cmd.clone().into();

        assert_eq!(cmd_struct.vendor_code, 0x90705);
        assert_eq!(cmd_struct.command, 0x47);
        assert_eq!(cmd_struct.length, 2);
        assert_eq!(cmd_struct.data[0], 0x55);
        assert_eq!(cmd_struct.data[1], 0xAA);

        assert_eq!(cmd, cmd_struct.into());
    }
}
