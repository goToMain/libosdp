use super::ConvertEndian;

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

impl From<libosdp_sys::osdp_pd_id> for PdId {
    fn from(value: libosdp_sys::osdp_pd_id) -> Self {
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

impl ConvertEndian for [u8; 4] {
    fn as_be(&self) -> u32 {
        ((self[0] as u32) << 24)
            | ((self[1] as u32) << 16)
            | ((self[2] as u32) << 8)
            | ((self[3] as u32) << 0)
    }

    fn as_le(&self) -> u32 {
        ((self[0] as u32) << 0)
            | ((self[1] as u32) << 8)
            | ((self[2] as u32) << 16)
            | ((self[3] as u32) << 24)
    }
}

impl ConvertEndian for (u8, u8, u8) {
    fn as_be(&self) -> u32 {
        ((self.0 as u32) << 24) | ((self.1 as u32) << 16) | ((self.2 as u32) << 8)
    }

    fn as_le(&self) -> u32 {
        ((self.0 as u32) << 0) | ((self.1 as u32) << 8) | ((self.2 as u32) << 16)
    }
}

impl From<PdId> for libosdp_sys::osdp_pd_id {
    fn from(value: PdId) -> Self {
        libosdp_sys::osdp_pd_id {
            version: value.version,
            model: value.model,
            vendor_code: value.vendor_code.as_le(),
            serial_number: value.serial_number.as_le(),
            firmware_version: value.firmware_version.as_le(),
        }
    }
}
