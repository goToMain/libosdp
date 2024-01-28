use alloc::ffi::CString;

use crate::channel::OsdpChannel;

use super::{OsdpFlag, PdCapability, PdId};

/// OSDP PD Information. This struct is used to describe a PD to LibOSDP
#[derive(Debug)]
pub struct PdInfo {
    name: CString,
    address: i32,
    baud_rate: i32,
    flags: OsdpFlag,
    id: PdId,
    cap: Vec<libosdp_sys::osdp_pd_cap>,
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
        scbk: [u8; 16],
    ) -> Self {
        let name = CString::new(name).unwrap();
        let cap = cap.into_iter().map(|c| c.into()).collect();
        Self {
            name,
            address,
            baud_rate,
            flags,
            id,
            cap,
            channel,
            scbk,
        }
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
        scbk: [u8; 16],
    ) -> Self {
        let name = CString::new(name).unwrap();
        Self {
            name,
            address,
            baud_rate,
            flags,
            id: PdId::default(),
            cap: vec![],
            channel,
            scbk,
        }
    }

    /// Returns the equivalent C struct
    pub fn as_struct(&self) -> libosdp_sys::osdp_pd_info_t {
        libosdp_sys::osdp_pd_info_t {
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
