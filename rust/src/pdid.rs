use crate::osdp_sys;


#[derive(Clone, Debug, Default)]
pub struct PdId {
    pub version: i32,
    pub model: i32,
    pub vendor_code: u32,
    pub serial_number: u32,
    pub firmware_version: u32,
}

impl From <osdp_sys::osdp_pd_id> for PdId {
    fn from(value: osdp_sys::osdp_pd_id) -> Self {
        Self {
            version: value.version,
            model: value.model,
            vendor_code: value.vendor_code,
            serial_number: value.serial_number,
            firmware_version: value.firmware_version,
        }
    }
}

impl From<PdId> for osdp_sys::osdp_pd_id {
    fn from(value: PdId) -> Self {
        osdp_sys::osdp_pd_id {
            version: value.version,
            model: value.model,
            vendor_code: value.vendor_code,
            serial_number: value.serial_number,
            firmware_version: value.firmware_version,
        }
    }
}
