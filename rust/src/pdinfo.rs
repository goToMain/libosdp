use crate::{
    osdp_sys,
    channel::OsdpChannel,
    error::OsdpError,
    pdid::PdId,
    pdcap::PdCapability
};
use std::{ffi::CString, str::FromStr};

bitflags::bitflags! {
    #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
    pub struct OsdpFlag: u32 {
        const EnforceSecure = osdp_sys::OSDP_FLAG_ENFORCE_SECURE;
        const InstallMode = osdp_sys::OSDP_FLAG_INSTALL_MODE;
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

#[derive(Debug)]
pub struct PdInfo {
    pub name: CString,
    pub address: i32,
    pub baud_rate: i32,
    pub flags: OsdpFlag,
    pub id: PdId,
    pub cap: Vec<osdp_sys::osdp_pd_cap>,
    pub channel: OsdpChannel,
    pub scbk: [u8; 16],
}

impl PdInfo {
    pub fn new(name: &str, address: i32, baud_rate: i32, flags: OsdpFlag, id: PdId, cap: Vec<PdCapability>, channel: OsdpChannel, scbk: [u8; 16]) -> Self {
        let name = CString::new(name).unwrap();
        let cap = cap.into_iter()
            .map(|c| { c.into() })
            .collect();
        Self { name, address, baud_rate, flags, id, cap, channel, scbk }
    }

    pub fn as_struct(&self) -> osdp_sys::osdp_pd_info_t {
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
