///! OSDP Command and Event Types
///!
///! This module is responsible to creating various types that can move between
///! C and Rust code.
mod commands;
mod events;
mod pdcap;
mod pdid;
mod pdinfo;

use crate::OsdpError;
use core::str::FromStr;

// Re-export for convenience
pub use commands::*;
pub use events::*;
pub use pdcap::*;
pub use pdid::*;
pub use pdinfo::*;

/// Trait to convert between BigEndian and LittleEndian types
pub trait ConvertEndian {
    /// Return `Self` as BigEndian
    fn as_be(&self) -> u32;
    /// Return `Self` as LittleEndian
    fn as_le(&self) -> u32;
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
        const EnforceSecure = libosdp_sys::OSDP_FLAG_ENFORCE_SECURE;

        /// When set, the PD would allow one session of secure channel to be
        /// setup with SCBK-D.
        ///
        /// In this mode, the PD is in a vulnerable state, the application is
        /// responsible for making sure that the device enters this mode only
        /// during controlled/provisioning-time environments.
        const InstallMode = libosdp_sys::OSDP_FLAG_INSTALL_MODE;

        /// When set, CP will not error and fail when the PD sends an unknown,
        /// unsolicited response. In PD mode this flag has no use.
        const IgnoreUnsolicited = libosdp_sys::OSDP_FLAG_IGN_UNSOLICITED;
    }
}

impl FromStr for OsdpFlag {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "EnforceSecure" => Ok(OsdpFlag::EnforceSecure),
            "InstallMode" => Ok(OsdpFlag::InstallMode),
            "IgnoreUnsolicited" => Ok(OsdpFlag::IgnoreUnsolicited),
            _ => Err(OsdpError::Parse(format!("OsdpFlag: {s}"))),
        }
    }
}
