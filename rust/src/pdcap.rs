use std::str::FromStr;
use crate::{error::OsdpError, osdp_sys};

#[derive(Clone, Debug, Default)]
pub struct PdCapEntry {
    compliance: u8,
    num_items: u8,
}

impl PdCapEntry {
    pub fn new(compliance: u8, num_items: u8) -> Self {
        Self { compliance, num_items }
    }
}

// From "Compliance:10,NumItems:20" to PdCapEntry { compliance: 10, num_items: 20 }
impl FromStr for PdCapEntry {
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

#[derive(Clone, Debug)]
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
    type Err = OsdpError;

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
        let e = PdCapEntry {
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
