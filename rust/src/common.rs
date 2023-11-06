use std::{
    ffi::{CString, CStr},
    str::FromStr
};
use crate::{
    libosdp,
    channel::OsdpChannel
};

pub struct PdId {
    pub version: i32,
    pub model: i32,
    pub vendor_code: u32,
    pub serial_number: u32,
    pub firmware_version: u32,
}

impl From <libosdp::osdp_pd_id> for PdId {
    fn from(value: libosdp::osdp_pd_id) -> Self {
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
    fn as_struct(&self) -> libosdp::osdp_pd_id {
        libosdp::osdp_pd_id {
            version: self.version,
            model: self.model,
            vendor_code: self.vendor_code,
            serial_number: self.serial_number,
            firmware_version: self.firmware_version,
        }
    }
}

pub struct PdCapEntry {
    pub compliance: u8,
    pub num_items: u8,
}

// From "Compliance:10,NumItems:20" to PdCapEntry { compliance: 10, num_items: 20 }
impl FromStr for PdCapEntry {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let (compliance, num_items);
        if let Some((compliance_str, num_items_str)) = s.split_once(',') {
            if let Some((key, val)) = compliance_str.split_once(':') {
                if key != "Compliance" {
                    return Err(anyhow::anyhow!("First key must be Compliance {s}"))
                }
                compliance = val.parse::<u8>().unwrap()
            } else {
                return Err(anyhow::anyhow!("Format error {s}"))
            }
            if let Some((key, val)) = num_items_str.split_once(':') {
                if key != "NumItems" {
                    return Err(anyhow::anyhow!("Second key must be NumItems {s}"))
                }
                num_items = val.parse::<u8>().unwrap()
            } else {
                return Err(anyhow::anyhow!("Format error {s}"))
            }
            Ok(Self{ compliance, num_items })
        } else {
            return Err(anyhow::anyhow!("Format error {s}"))
        }
    }
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

impl FromStr for PdCapability {
    type Err = anyhow::Error;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if let Some((cap, ent)) = s.split_once(':') {
            match cap {
                "ContactStatusMonitoring" => Ok(PdCapability::ContactStatusMonitoring(PdCapEntry::from_str(ent)?)),
                "OutputControl" => Ok(PdCapability::OutputControl(PdCapEntry::from_str(ent)?)),
                "CardDataFormat" => Ok(PdCapability::CardDataFormat(PdCapEntry::from_str(ent)?)),
                "LedControl" => Ok(PdCapability::LedControl(PdCapEntry::from_str(ent)?)),
                "AudibleOutput" => Ok(PdCapability::AudibleOutput(PdCapEntry::from_str(ent)?)),
                "TextOutput" => Ok(PdCapability::TextOutput(PdCapEntry::from_str(ent)?)),
                "TimeKeeping" => Ok(PdCapability::TimeKeeping(PdCapEntry::from_str(ent)?)),
                "CheckCharacterSupport" => Ok(PdCapability::CheckCharacterSupport(PdCapEntry::from_str(ent)?)),
                "CommunicationSecurity" => Ok(PdCapability::CommunicationSecurity(PdCapEntry::from_str(ent)?)),
                "ReceiveBufferSize" => Ok(PdCapability::ReceiveBufferSize(PdCapEntry::from_str(ent)?)),
                "LargestCombinedMessage" => Ok(PdCapability::LargestCombinedMessage(PdCapEntry::from_str(ent)?)),
                "SmartCardSupport" => Ok(PdCapability::SmartCardSupport(PdCapEntry::from_str(ent)?)),
                "Readers" => Ok(PdCapability::Readers(PdCapEntry::from_str(ent)?)),
                "Biometrics" => Ok(PdCapability::Biometrics(PdCapEntry::from_str(ent)?)),
                _ => Err(anyhow::anyhow!("Unable to parse PdCapability: {s}")),
            }
        } else {
            Err(anyhow::anyhow!("Unable to parse PdCapability: {s}"))
        }
    }
}

impl From<libosdp::osdp_pd_cap> for PdCapability {
    fn from(value: libosdp::osdp_pd_cap) -> Self {
        let function_code = value.function_code as u32;
        let e = PdCapEntry {
            compliance: value.compliance_level,
            num_items: value.num_items
        };
        match function_code {
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING => PdCapability::ContactStatusMonitoring(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL => PdCapability::OutputControl(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT => PdCapability::CardDataFormat(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL => PdCapability::LedControl(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT => PdCapability::AudibleOutput(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT => PdCapability::TextOutput(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING => PdCapability::TimeKeeping(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT => PdCapability::CheckCharacterSupport(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY => PdCapability::CommunicationSecurity(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE => PdCapability::ReceiveBufferSize(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE => PdCapability::LargestCombinedMessage(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT => PdCapability::SmartCardSupport(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS => PdCapability::Readers(e),
            libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS => PdCapability::Biometrics(e),
            _ => panic!("Unknown function code"),
        }
    }
}

impl PdCapability {
    pub fn as_u8(&self) -> u8 {
        match self {
            PdCapability::ContactStatusMonitoring(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING as u8,
            PdCapability::OutputControl(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL as u8,
            PdCapability::CardDataFormat(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT as u8,
            PdCapability::LedControl(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL as u8,
            PdCapability::AudibleOutput(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT as u8,
            PdCapability::TextOutput(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT as u8,
            PdCapability::TimeKeeping(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING as u8,
            PdCapability::CheckCharacterSupport(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT as u8,
            PdCapability::CommunicationSecurity(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY as u8,
            PdCapability::ReceiveBufferSize(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE as u8,
            PdCapability::LargestCombinedMessage(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE as u8,
            PdCapability::SmartCardSupport(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT as u8,
            PdCapability::Readers(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS as u8,
            PdCapability::Biometrics(_) => libosdp::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS as u8,
        }
    }

    pub fn as_struct(&self) -> libosdp::osdp_pd_cap {
        match self {
            PdCapability::ContactStatusMonitoring(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::OutputControl(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CardDataFormat(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LedControl(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,

            },
            PdCapability::AudibleOutput(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TextOutput(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TimeKeeping(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CheckCharacterSupport(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CommunicationSecurity(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::ReceiveBufferSize(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LargestCombinedMessage(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::SmartCardSupport(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Readers(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Biometrics(e) => libosdp::osdp_pd_cap {
                function_code: self.as_u8(),
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
        }
    }
}

bitflags::bitflags! {
    #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
    pub struct OsdpFlag: u32 {
        const EnforceSecure = libosdp::OSDP_FLAG_ENFORCE_SECURE;
        const InstallMode = libosdp::OSDP_FLAG_INSTALL_MODE;
        const IgnoreUnsolicited = libosdp::OSDP_FLAG_IGN_UNSOLICITED;
    }
}

pub struct PdInfo {
    pub name: CString,
    pub address: i32,
    pub baud_rate: i32,
    pub flags: OsdpFlag,
    pub id: PdId,
    pub cap: Vec<libosdp::osdp_pd_cap>,
    pub channel: OsdpChannel,
    pub scbk: [u8; 16],
}

impl<'a> PdInfo {
    pub fn new(name: &str, address: i32, baud_rate: i32, flags: OsdpFlag, id: PdId, cap: Vec<PdCapability>, channel: OsdpChannel, scbk: [u8; 16]) -> Self {
        Self {
            name: CString::new(name).unwrap(),
            address,
            baud_rate,
            flags,
            id,
            cap: cap.iter().map(|c| -> libosdp::osdp_pd_cap { c.as_struct() }).collect(),
            channel,
            scbk,
        }
    }

    pub fn as_struct(&mut self) -> libosdp::osdp_pd_info_t {
        libosdp::osdp_pd_info_t {
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

pub fn cstr_to_string(s: *const ::std::os::raw::c_char) -> String {
    let s = unsafe {
        CStr::from_ptr(s)
    };
    s.to_str().unwrap().to_owned()
}
