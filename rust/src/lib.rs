//! # LibOSDP - Open Supervised Device Protocol Library
//!
//! This is an open source implementation of IEC 60839-11-5 Open Supervised
//! Device Protocol (OSDP). The protocol is intended to improve interoperability
//! among access control and security products. It supports Secure Channel (SC)
//! for encrypted and authenticated communication between configured devices.
//!
//! OSDP describes the communication protocol for interfacing one or more
//! Peripheral Devices (PD) to a Control Panel (CP) over a two-wire RS-485
//! multi-drop serial communication channel. Nevertheless, this protocol can be
//! used to transfer secure data over any stream based physical channel. Read
//! more about OSDP [here][1].
//!
//! This protocol is developed and maintained by [Security Industry Association][2]
//! (SIA).
//!
//! ## Salient Features of LibOSDP
//!
//!   - Supports secure channel communication (AES-128)
//!   - Can be used to setup a PD or CP mode of operation
//!   - Exposes a well defined contract though a single header file
//!   - No run-time memory allocation. All memory is allocated at init-time
//!   - No external dependencies (for ease of cross compilation)
//!   - Fully non-blocking, asynchronous design
//!   - Provides Python3 and Rust bindings for the C library for faster
//!     testing/integration
//!
//! ## Quick start
//!
//! #### Control Panel:
//!
//! A simplified (non-working) CP implementation:
//!
//! ```rust
//! let pd_info = vec! [ PdInfo::new(...), ... ];
//! let mut cp = ControlPanel::new(&mut pd_info)?;
//! cp.set_event_callback(|pd, event| {
//!     println!("Received event from {pd}: {:?}", event);
//! });
//! loop {
//!     cp.refresh();
//!     cp.send_command(0, OsdpCommand::new(...));
//! }
//! ```
//!
//! #### Peripheral Device:
//!
//! A simplified (non-working) PD implementation:
//!
//! ```rust
//! let pd_info = PdInfo::new(...);
//! let mut pd = PeripheralDevice::new(&mut pd_info)?;
//! pd.set_command_callback(|cmd| {
//!     println!("Received command {:?}", cmd);
//! });
//! loop {
//!     pd.refresh();
//!     cp.notify_event(OsdpEvent::new(...));
//! }
//! ```
//!
//! [1]: https://libosdp.sidcha.dev/protocol/
//! [2]: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/

#![warn(missing_debug_implementations)]
#![warn(rust_2018_idioms)]
#![warn(missing_docs)]

pub mod cp;
pub mod pd;
pub mod commands;
pub mod events;
pub mod channel;
pub mod file;
mod osdp_sys;

use std::{str::FromStr, ffi::CString, sync::Mutex};
use channel::OsdpChannel;
use once_cell::sync::Lazy;
use thiserror::Error;

/// OSDP public errors
#[derive(Debug, Default, Error)]
pub enum OsdpError {
    /// PD info error
    #[error("Invalid PdInfo {0}")]
    PdInfo(&'static str),

    /// Command build/send error
    #[error("Invalid OsdpCommand")]
    Command,

    /// Event build/send error
    #[error("Invalid OsdpEvent")]
    Event,

    /// PD/CP status query error
    #[error("Failed to query {0} from device")]
    Query(&'static str),
    #[error("File transfer failed: {0}")]

    /// File transfer errors
    FileTransfer(&'static str),

    /// CP/PD device setup failed.
    #[error("Failed to setup device")]
    Setup,

    /// String parse error
    #[error("Type {0} parse error")]
    Parse(String),

    /// IO Error
    #[error("IO Error")]
    IO(#[from] std::io::Error),

    /// Unknown error
    #[default]
    #[error("Unknown/Unspecified error")]
    Unknown,
}

impl From<std::convert::Infallible> for OsdpError {
    fn from(_: std::convert::Infallible) -> Self {
        unreachable!()
    }
}

fn cstr_to_string(s: *const ::std::os::raw::c_char) -> String {
    let s = unsafe {
        std::ffi::CStr::from_ptr(s)
    };
    s.to_str().unwrap().to_owned()
}

static VERSION: Lazy<Mutex<String>> = Lazy::new(|| {
    let s = unsafe { osdp_sys::osdp_get_version() };
    Mutex::new(cstr_to_string(s))
});
static SOURCE_INFO: Lazy<Mutex<String>> = Lazy::new(|| {
    let s = unsafe { osdp_sys::osdp_get_source_info() };
    Mutex::new(cstr_to_string(s))
});

/// Get LibOSDP version
pub fn get_version() -> String {
    VERSION.lock().unwrap().clone()
}

/// Get LibOSDP source info string
pub fn get_source_info() -> String {
    SOURCE_INFO.lock().unwrap().clone()
}

/// PD ID information advertised by the PD.
#[derive(Clone, Debug, Default)]
pub struct PdId {
    /// 1-Byte Manufacturer's version number
    pub version: i32,
    /// 1-byte Manufacturer's model number
    pub model: i32,
    /// 3-bytes IEEE assigned OUI
    pub vendor_code: (u8, u8, u8),
    /// 4-byte serial number for the PD
    pub serial_number: [u8; 4],
    /// 3-byte version (major, minor, build)
    pub firmware_version: (u8, u8, u8),
}

impl PdId {
    /// Create an instance of PdId from crate metadata
    pub fn from_number(num: u8) -> Self {
        let v_major: u8 = env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap();
        let v_minor: u8 = env!("CARGO_PKG_VERSION_MINOR").parse().unwrap();
        let v_patch: u8 = env!("CARGO_PKG_VERSION_PATCH").parse().unwrap();
        Self {
            version: 0x74,
            model: 0x23,
            vendor_code: (0xA0, 0xB2, 0xFE),
            serial_number: [0x00, 0x00, 0x00, num],
            firmware_version: (v_major, v_minor, v_patch),
        }
    }
}

impl From <osdp_sys::osdp_pd_id> for PdId {
    fn from(value: osdp_sys::osdp_pd_id) -> Self {
        let bytes = value.vendor_code.to_le_bytes();
        let vendor_code: (u8, u8, u8) = (bytes[0], bytes[1], bytes[2]);
        let bytes = value.firmware_version.to_le_bytes();
        let firmware_version = (bytes[0], bytes[1], bytes[2]);
        Self {
            version: value.version,
            model: value.model,
            vendor_code,
            serial_number: value.serial_number.to_be_bytes(),
            firmware_version,
        }
    }
}

trait ConvertEndian {
    fn as_be(&self) -> u32;
    fn as_le(&self) -> u32;
}

impl ConvertEndian for [u8; 4] {
    fn as_be(&self) -> u32 {
        ((self[0] as u32) << 24) |
        ((self[1] as u32) << 16) |
        ((self[2] as u32) <<  8) |
        ((self[3] as u32) <<  0)
    }

    fn as_le(&self) -> u32 {
        ((self[0] as u32) <<  0) |
        ((self[1] as u32) <<  8) |
        ((self[2] as u32) << 16) |
        ((self[3] as u32) << 24)
    }
}

impl ConvertEndian for (u8, u8, u8) {
    fn as_be(&self) -> u32 {
        ((self.0 as u32) << 24) |
        ((self.1 as u32) << 16) |
        ((self.2 as u32) <<  8)
    }

    fn as_le(&self) -> u32 {
        ((self.0 as u32) <<  0) |
        ((self.1 as u32) <<  8) |
        ((self.2 as u32) << 16)
    }
}

impl From<PdId> for osdp_sys::osdp_pd_id {
    fn from(value: PdId) -> Self {
        osdp_sys::osdp_pd_id {
            version: value.version,
            model: value.model,
            vendor_code: value.vendor_code.as_le(),
            serial_number: value.serial_number.as_le(),
            firmware_version: value.firmware_version.as_le(),
        }
    }
}


bitflags::bitflags! {
    /// OSDP setup flags
    #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
    pub struct OsdpFlag: u32 {
        /// Make security conscious assumptions where possible. Fail where these
        /// assumptions don't hold. The following restrictions are enforced in
        /// this mode:
        ///
        /// - Don't allow use of SCBK-D (implies no INSTALL_MODE)
        /// - Assume that a KEYSET was successful at an earlier time
        /// - Disallow master key based SCBK derivation
        const EnforceSecure = osdp_sys::OSDP_FLAG_ENFORCE_SECURE;

        /// When set, the PD would allow one session of secure channel to be
        /// setup with SCBK-D.
        ///
        /// In this mode, the PD is in a vulnerable state, the application is
        /// responsible for making sure that the device enters this mode only
        /// during controlled/provisioning-time environments.
        const InstallMode = osdp_sys::OSDP_FLAG_INSTALL_MODE;

        /// When set, CP will not error and fail when the PD sends an unknown,
        /// unsolicited response. In PD mode this flag has no use.
        const IgnoreUnsolicited = osdp_sys::OSDP_FLAG_IGN_UNSOLICITED;
    }
}

impl FromStr for OsdpFlag {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "EnforceSecure" => Ok(OsdpFlag::EnforceSecure),
            "InstallMode" => Ok(OsdpFlag::InstallMode),
            "IgnoreUnsolicited" => Ok(OsdpFlag::IgnoreUnsolicited),
            _ => Err(OsdpError::Parse(format!("OsdpFlag: {s}")))
        }
    }
}

/// PD capability entity to be used inside [`PdCapability`]
#[derive(Clone, Debug, Default)]
pub struct PdCapEntity {
    compliance: u8,
    num_items: u8,
}

impl PdCapEntity {
    /// Create a new capability entity to be used inside [`PdCapability`].
    ///
    /// # Arguments
    ///
    /// * `compliance` - A number that indicates what the PD can do with this
    ///   capability. This number means different things for different
    ///   [`PdCapability`].
    /// * `num_items` - number of units of such capability in the PD. For
    ///    LED capability ([`PdCapability::LedControl`]), this would indicate
    ///    the number of controllable LEDs available on this PD.
    pub fn new(compliance: u8, num_items: u8) -> Self {
        Self { compliance, num_items }
    }
}

// From "Compliance:10,NumItems:20" to PdCapEntry { compliance: 10, num_items: 20 }
impl FromStr for PdCapEntity {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let (compliance, num_items);
        if let Some((compliance_str, num_items_str)) = s.split_once(',') {
            if let Some((key, val)) = compliance_str.split_once(':') {
                if key != "Compliance" {
                    return Err(OsdpError::Parse(format!("PdCapEntry: {s}")))
                }
                compliance = val.parse::<u8>().unwrap()
            } else {
                return Err(OsdpError::Parse(format!("PdCapEntry: {s}")))
            }
            if let Some((key, val)) = num_items_str.split_once(':') {
                if key != "NumItems" {
                    return Err(OsdpError::Parse(format!("PdCapEntry: {s}")))
                }
                num_items = val.parse::<u8>().unwrap()
            } else {
                return Err(OsdpError::Parse(format!("PdCapEntry: {s}")))
            }
            Ok(Self{ compliance, num_items })
        } else {
            return Err(OsdpError::Parse(format!("PdCapEntry: {s}")))
        }
    }
}

/// OSDP defined PD capabilities. PDs expose/advertise features they support to
/// the CP by means of "capabilities".
#[derive(Clone, Debug)]
pub enum PdCapability {
	/// This function indicates the ability to monitor the status of a switch
    /// using a two-wire electrical connection between the PD and the switch.
    /// The on/off position of the switch indicates the state of an external
    /// device.
    ///
	/// The PD may simply resolve all circuit states to an open/closed
	/// status, or it may implement supervision of the monitoring circuit.
	/// A supervised circuit is able to indicate circuit fault status in
	/// addition to open/closed status.
    ContactStatusMonitoring(PdCapEntity),

    /// This function provides a switched output, typically in the form of a
    /// relay. The Output has two states: active or inactive. The Control Panel
    /// (CP) can directly set the Output's state, or, if the PD supports timed
    /// operations, the CP can specify a time period for the activation of the
    /// Output.
    OutputControl(PdCapEntity),

    /// This capability indicates the form of the card data is presented to the
    /// Control Panel.
    CardDataFormat(PdCapEntity),

    /// This capability indicates the presence of and type of LEDs.
    LedControl(PdCapEntity),

    /// This capability indicates the presence of and type of an audible
    /// annunciator (buzzer or similar tone generator).
    AudibleOutput(PdCapEntity),

    /// This capability indicates that the PD supports a text display emulating
    /// character-based display terminals.
    TextOutput(PdCapEntity),

    /// This capability indicates that the type of date and time awareness or
    /// time keeping ability of the PD.
    TimeKeeping(PdCapEntity),

    /// All PDs must be able to support the checksum mode. This capability
    /// indicates if the PD is capable of supporting CRC mode.
    CheckCharacterSupport(PdCapEntity),

    /// This capability indicates the extent to which the PD supports
    /// communication security (Secure Channel Communication)
    CommunicationSecurity(PdCapEntity),

    /// This capability indicates the maximum size single message the PD can
    /// receive.
    ReceiveBufferSize(PdCapEntity),

    /// This capability indicates the maximum size multi-part message which the
    /// PD can handle.
    LargestCombinedMessage(PdCapEntity),

    /// This capability indicates whether the PD supports the transparent mode
    /// used for communicating directly with a smart card.
    SmartCardSupport(PdCapEntity),

    /// This capability indicates the number of credential reader devices
    /// present. Compliance levels are bit fields to be assigned as needed.
    Readers(PdCapEntity),

    /// This capability indicates the ability of the reader to handle biometric
    /// input.
    Biometrics(PdCapEntity),
}

impl FromStr for PdCapability {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if let Some((cap, ent)) = s.split_once(':') {
            match cap {
                "ContactStatusMonitoring" => Ok(PdCapability::ContactStatusMonitoring(PdCapEntity::from_str(ent)?)),
                "OutputControl" => Ok(PdCapability::OutputControl(PdCapEntity::from_str(ent)?)),
                "CardDataFormat" => Ok(PdCapability::CardDataFormat(PdCapEntity::from_str(ent)?)),
                "LedControl" => Ok(PdCapability::LedControl(PdCapEntity::from_str(ent)?)),
                "AudibleOutput" => Ok(PdCapability::AudibleOutput(PdCapEntity::from_str(ent)?)),
                "TextOutput" => Ok(PdCapability::TextOutput(PdCapEntity::from_str(ent)?)),
                "TimeKeeping" => Ok(PdCapability::TimeKeeping(PdCapEntity::from_str(ent)?)),
                "CheckCharacterSupport" => Ok(PdCapability::CheckCharacterSupport(PdCapEntity::from_str(ent)?)),
                "CommunicationSecurity" => Ok(PdCapability::CommunicationSecurity(PdCapEntity::from_str(ent)?)),
                "ReceiveBufferSize" => Ok(PdCapability::ReceiveBufferSize(PdCapEntity::from_str(ent)?)),
                "LargestCombinedMessage" => Ok(PdCapability::LargestCombinedMessage(PdCapEntity::from_str(ent)?)),
                "SmartCardSupport" => Ok(PdCapability::SmartCardSupport(PdCapEntity::from_str(ent)?)),
                "Readers" => Ok(PdCapability::Readers(PdCapEntity::from_str(ent)?)),
                "Biometrics" => Ok(PdCapability::Biometrics(PdCapEntity::from_str(ent)?)),
                _ => Err(OsdpError::Parse(format!("PdCapability: {s}"))),
            }
        } else {
            Err(OsdpError::Parse(format!("PdCapability: {s}")))
        }
    }
}

impl From<osdp_sys::osdp_pd_cap> for PdCapability {
    fn from(value: osdp_sys::osdp_pd_cap) -> Self {
        let function_code = value.function_code as u32;
        let e = PdCapEntity {
            compliance: value.compliance_level,
            num_items: value.num_items
        };
        match function_code {
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING => PdCapability::ContactStatusMonitoring(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL => PdCapability::OutputControl(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT => PdCapability::CardDataFormat(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL => PdCapability::LedControl(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT => PdCapability::AudibleOutput(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT => PdCapability::TextOutput(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING => PdCapability::TimeKeeping(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT => PdCapability::CheckCharacterSupport(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY => PdCapability::CommunicationSecurity(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE => PdCapability::ReceiveBufferSize(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE => PdCapability::LargestCombinedMessage(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT => PdCapability::SmartCardSupport(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS => PdCapability::Readers(e),
            osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS => PdCapability::Biometrics(e),
            _ => panic!("Unknown function code"),
        }
    }
}

impl Into<u8> for PdCapability {
    fn into(self) -> u8 {
        match self {
            PdCapability::ContactStatusMonitoring(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING as u8,
            PdCapability::OutputControl(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL as u8,
            PdCapability::CardDataFormat(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT as u8,
            PdCapability::LedControl(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL as u8,
            PdCapability::AudibleOutput(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT as u8,
            PdCapability::TextOutput(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT as u8,
            PdCapability::TimeKeeping(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING as u8,
            PdCapability::CheckCharacterSupport(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT as u8,
            PdCapability::CommunicationSecurity(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY as u8,
            PdCapability::ReceiveBufferSize(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE as u8,
            PdCapability::LargestCombinedMessage(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE as u8,
            PdCapability::SmartCardSupport(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT as u8,
            PdCapability::Readers(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS as u8,
            PdCapability::Biometrics(_) => osdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS as u8,
        }
    }
}

impl From<PdCapability> for osdp_sys::osdp_pd_cap {
    fn from(value: PdCapability) -> Self {
        let function_code: u8 = value.clone().into();
        match value {
            PdCapability::ContactStatusMonitoring(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::OutputControl(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CardDataFormat(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LedControl(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,

            },
            PdCapability::AudibleOutput(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TextOutput(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TimeKeeping(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CheckCharacterSupport(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CommunicationSecurity(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::ReceiveBufferSize(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LargestCombinedMessage(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::SmartCardSupport(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Readers(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Biometrics(e) => osdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
        }
    }
}

/// OSDP PD Information. This struct is used to describe a PD to LibOSDP
#[derive(Debug)]
pub struct PdInfo {
    name: CString,
    address: i32,
    baud_rate: i32,
    flags: OsdpFlag,
    id: PdId,
    cap: Vec<osdp_sys::osdp_pd_cap>,
    channel: OsdpChannel,
    scbk: [u8; 16],
}

impl PdInfo {
    /// Create an instance of PdInfo struct for Peripheral Device (PD)
    ///
    /// # Arguments
    ///
    /// * `name` - User provided name for this PD (log messages include this name)
    /// * `address` - 7 bit PD address. the rest of the bits are ignored. The
    ///   special address 0x7F is used for broadcast. So there can be 2^7-1
    ///   devices on a multi-drop channel
    /// * `baud_rate` - Can be one of 9600/19200/38400/57600/115200/230400
    /// * `flags` - Used to modify the way the context is setup.
    /// * `id` - Static information that the PD reports to the CP when it
    ///    received a `CMD_ID`. These information must be populated by a PD
    ///    application.
    /// * `cap` - A vector of [`PdCapability`] entries (PD mode)
    /// * `channel` - Osdp communication channel.
    /// * `scbk` - Secure channel base key data
    pub fn for_pd(
        name: &str,
        address: i32,
        baud_rate: i32,
        flags: OsdpFlag,
        id: PdId,
        cap: Vec<PdCapability>,
        channel: OsdpChannel,
        scbk: [u8; 16]
    ) -> Self {
        let name = CString::new(name).unwrap();
        let cap = cap.into_iter()
            .map(|c| { c.into() })
            .collect();
        Self { name, address, baud_rate, flags, id, cap, channel, scbk }
    }

    /// Create an instance of PdInfo struct for Control Panel (CP)
    ///
    /// # Arguments
    ///
    /// * `name` - User provided name for this PD (log messages include this name)
    /// * `address` - 7 bit PD address. the rest of the bits are ignored. The
    ///   special address 0x7F is used for broadcast. So there can be 2^7-1
    ///   devices on a multi-drop channel
    /// * `baud_rate` - Can be one of 9600/19200/38400/57600/115200/230400
    /// * `flags` - Used to modify the way the context is setup.
    /// * `channel` - Osdp communication channel.
    /// * `scbk` - Secure channel base key data
    pub fn for_cp(
        name: &str,
        address: i32,
        baud_rate: i32,
        flags: OsdpFlag,
        channel: OsdpChannel,
        scbk: [u8; 16]
    ) -> Self {
        let name = CString::new(name).unwrap();
        Self { name, address, baud_rate, flags, id: PdId::default(), cap: vec![], channel, scbk }
    }

    fn as_struct(&self) -> osdp_sys::osdp_pd_info_t {
        osdp_sys::osdp_pd_info_t {
            name: self.name.as_ptr(),
            baud_rate: self.baud_rate,
            address: self.address,
            flags: self.flags.bits() as i32,
            id: self.id.clone().into(),
            cap: self.cap.as_ptr(),
            channel: self.channel.as_struct(),
            scbk: self.scbk.as_ptr(),
        }
    }
}
