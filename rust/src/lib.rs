#![cfg_attr(not(feature = "std"), no_std)]
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
//! ```ignore
//! use libosdp::{PdInfo, ControlPanel, OsdpCommand, OsdpCommandCardRead};
//!
//! let pd_info = vec! [ PdInfo::new(...), ... ];
//! let mut cp = ControlPanel::new(pd_info).unwrap();
//! cp.set_event_callback(|pd, event| {
//!     println!("Received event from {pd}: {:?}", event);
//!     0
//! });
//! loop {
//!     cp.refresh();
//!     let c = OsdpCommandCardRead::new_ascii(vec![0x55, 0xAA]);
//!     cp.send_command(0, OsdpCommand::new(c));
//! }
//! ```
//!
//! #### Peripheral Device:
//!
//! A simplified (non-working) PD implementation:
//!
//! ```ignore
//! use libosdp::{PdInfo, pd::PeripheralDevice, events::OsdpEvent};
//!
//! let pd_info = PdInfo::new(...);
//! let mut pd = PeripheralDevice::new(&mut pd_info).unwrap();
//! pd.set_command_callback(|cmd| {
//!     println!("Received command {:?}", cmd);
//!     0
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

extern crate alloc;

pub mod channel;
mod cp;
pub mod file;
#[cfg(feature = "std")]
mod pd;
mod types;

#[allow(unused_imports)]
use alloc::{
    borrow::ToOwned, boxed::Box, ffi::CString, format, str::FromStr, string::String, sync::Arc,
    vec, vec::Vec,
};
use once_cell::sync::Lazy;
#[cfg(feature = "std")]
use thiserror::Error;

pub use cp::ControlPanel;
pub use pd::PeripheralDevice;
pub use types::*;

/// OSDP public errors
#[derive(Debug, Default)]
#[cfg_attr(feature = "std", derive(Error))]
pub enum OsdpError {
    /// PD info error
    #[cfg_attr(feature = "std", error("Invalid PdInfo {0}"))]
    PdInfo(&'static str),

    /// Command build/send error
    #[cfg_attr(feature = "std", error("Invalid OsdpCommand"))]
    Command,

    /// Event build/send error
    #[cfg_attr(feature = "std", error("Invalid OsdpEvent"))]
    Event,

    /// PD/CP status query error
    #[cfg_attr(feature = "std", error("Failed to query {0} from device"))]
    Query(&'static str),

    /// File transfer errors
    #[cfg_attr(feature = "std", error("File transfer failed: {0}"))]
    FileTransfer(&'static str),

    /// CP/PD device setup failed.
    #[cfg_attr(feature = "std", error("Failed to setup device"))]
    Setup,

    /// String parse error
    #[cfg_attr(feature = "std", error("Type {0} parse error"))]
    Parse(String),

    /// OSDP channel error
    #[cfg_attr(feature = "std", error("Channel error: {0}"))]
    Channel(&'static str),

    /// IO Error
    #[cfg(feature = "std")]
    #[error("IO Error")]
    IO(#[from] std::io::Error),
    /// IO Error
    #[cfg(not(feature = "std"))]
    IO(Box<dyn embedded_io::Error>),

    /// Unknown error
    #[default]
    #[cfg_attr(feature = "std", error("Unknown/Unspecified error"))]
    Unknown,
}

impl From<core::convert::Infallible> for OsdpError {
    fn from(_: core::convert::Infallible) -> Self {
        unreachable!()
    }
}

fn cstr_to_string(s: *const ::core::ffi::c_char) -> String {
    let s = unsafe { core::ffi::CStr::from_ptr(s) };
    s.to_str().unwrap().to_owned()
}

static VERSION: Lazy<Arc<String>> = Lazy::new(|| {
    let s = unsafe { libosdp_sys::osdp_get_version() };
    Arc::new(cstr_to_string(s))
});

static SOURCE_INFO: Lazy<Arc<String>> = Lazy::new(|| {
    let s = unsafe { libosdp_sys::osdp_get_source_info() };
    Arc::new(cstr_to_string(s))
});

/// Get LibOSDP version
pub fn get_version() -> String {
    VERSION.as_ref().clone()
}

/// Get LibOSDP source info string
pub fn get_source_info() -> String {
    SOURCE_INFO.as_ref().clone()
}
