use std::ffi::CString;

use crate::channel::OsdpChannel;

pub struct PdId {
    pub version: i32,
    pub model: i32,
    pub vendor_code: u32,
    pub serial_number: u32,
    pub firmware_version: u32,
}

impl From <crate::osdp_pd_id> for PdId {
    fn from(value: crate::osdp_pd_id) -> Self {
        Self {
            version: value.version, 
            model: value.model, 
            vendor_code: value.vendor_code, 
            serial_number: value.serial_number, 
            firmware_version: value.firmware_version, 
        }
    }
}

impl PdId {
    fn as_struct(&self) -> crate::osdp_pd_id {
        crate::osdp_pd_id {
            version: self.version,
            model: self.model,
            vendor_code: self.vendor_code,
            serial_number: self.serial_number,
            firmware_version: self.firmware_version,
        }
    }
}

pub struct PdCapEntry {
    compliance: u8,
    num_items: u8,
}

pub enum PdCapability {
    ContactStatusMonitoring(PdCapEntry),
    OutputControl(PdCapEntry),
    CardDataFormat(PdCapEntry),
    LedControl(PdCapEntry),
    AudibleOutput(PdCapEntry),
    TextOutput(PdCapEntry),
    TimeKeeping(PdCapEntry),
    CheckCharacterSupport(PdCapEntry),
    CommunicationSecurity(PdCapEntry),
    ReceiveBufferSize(PdCapEntry),
    LargestCombinedMessage(PdCapEntry),
    SmartCardSupport(PdCapEntry),
    Readers(PdCapEntry),
    Biometrics(PdCapEntry),
}

impl PdCapability {
    pub fn as_struct(&self) -> crate::osdp_pd_cap {
        match self {
            PdCapability::ContactStatusMonitoring(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::OutputControl(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CardDataFormat(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LedControl(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,

            },
            PdCapability::AudibleOutput(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TextOutput(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TimeKeeping(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CheckCharacterSupport(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CommunicationSecurity(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::ReceiveBufferSize(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LargestCombinedMessage(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::SmartCardSupport(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Readers(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Biometrics(e) => crate::osdp_pd_cap {
                function_code: crate::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS as u8,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
        }
    }
}

bitflags::bitflags! {
    #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
    pub struct OsdpFlags: u32 {
        const EnforceSecure = crate::OSDP_FLAG_ENFORCE_SECURE;
        const InstallMode = crate::OSDP_FLAG_INSTALL_MODE;
        const IgnoreUnsolicited = crate::OSDP_FLAG_IGN_UNSOLICITED;
    }
}

pub struct PdInfo<'a> {
    pub name: CString,
    pub address: i32,
    pub baud_rate: i32,
    pub flags: OsdpFlags,
    pub id: PdId,
    pub cap: Vec<crate::osdp_pd_cap>,
    pub channel: OsdpChannel<'a>,
    pub scbk: [u8; 16],
}

impl<'a> PdInfo<'a> {

    pub fn new(name: &str, address: i32, baud_rate: i32, flags: OsdpFlags, id: PdId, cap: Vec<PdCapability>, channel: OsdpChannel<'a>, scbk: [u8; 16]) -> Self {
        Self {
            name: CString::new(name).unwrap(),
            address,
            baud_rate,
            flags,
            id,
            cap: cap.iter().map(|c| -> crate::osdp_pd_cap { c.as_struct() }).collect(),
            channel,
            scbk,
        }
    }

    pub fn as_struct(&mut self) -> crate::osdp_pd_info_t {
        crate::osdp_pd_info_t {
            name: self.name.as_ptr(),
            baud_rate: self.baud_rate,
            address: self.address,
            flags: self.flags.bits() as i32,
            id: self.id.as_struct(),
            cap: self.cap.as_mut_ptr(),
            channel: self.channel.as_struct(),
            scbk: self.scbk.as_mut_ptr(),
        }
    }
}
