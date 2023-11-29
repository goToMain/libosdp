//! LibOSDP
#![warn(missing_debug_implementations)]
#![warn(rust_2018_idioms)]
#![warn(missing_docs)]

pub mod osdp_sys;
pub mod cp;
pub mod pd;
pub mod commands;
pub mod events;
pub mod channel;
pub mod file;
pub mod pdcap;

use std::{str::FromStr, ffi::CString, sync::Mutex};
use channel::OsdpChannel;
use once_cell::sync::Lazy;
use pdcap::PdCapability;
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
        /// ENFORCE_SECURE: Make security conscious assumptions where possible.
        /// Fail where these assumptions don't hold. The following restrictions
        /// are enforced in this mode:
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
