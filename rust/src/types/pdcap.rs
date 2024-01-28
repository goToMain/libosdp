use core::str::FromStr;

use crate::OsdpError;

/// PD capability entity to be used inside [`PdCapability`]
#[derive(Clone, Debug, Default)]
pub struct PdCapEntity {
    compliance: u8,
    num_items: u8,
}

impl PdCapEntity {
    /// Create a new capability entity to be used inside [`PdCapability`].
    ///
    /// # Arguments
    ///
    /// * `compliance` - A number that indicates what the PD can do with this
    ///   capability. This number means different things for different
    ///   [`PdCapability`].
    /// * `num_items` - number of units of such capability in the PD. For
    ///    LED capability ([`PdCapability::LedControl`]), this would indicate
    ///    the number of controllable LEDs available on this PD.
    pub fn new(compliance: u8, num_items: u8) -> Self {
        Self {
            compliance,
            num_items,
        }
    }
}

// From "Compliance:10,NumItems:20" to PdCapEntry { compliance: 10, num_items: 20 }
impl FromStr for PdCapEntity {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let (compliance, num_items);
        if let Some((compliance_str, num_items_str)) = s.split_once(',') {
            if let Some((key, val)) = compliance_str.split_once(':') {
                if key != "Compliance" {
                    return Err(OsdpError::Parse(format!("PdCapEntry: {s}")));
                }
                compliance = val.parse::<u8>().unwrap()
            } else {
                return Err(OsdpError::Parse(format!("PdCapEntry: {s}")));
            }
            if let Some((key, val)) = num_items_str.split_once(':') {
                if key != "NumItems" {
                    return Err(OsdpError::Parse(format!("PdCapEntry: {s}")));
                }
                num_items = val.parse::<u8>().unwrap()
            } else {
                return Err(OsdpError::Parse(format!("PdCapEntry: {s}")));
            }
            Ok(Self {
                compliance,
                num_items,
            })
        } else {
            return Err(OsdpError::Parse(format!("PdCapEntry: {s}")));
        }
    }
}

/// OSDP defined PD capabilities. PDs expose/advertise features they support to
/// the CP by means of "capabilities".
#[derive(Clone, Debug)]
pub enum PdCapability {
    /// This function indicates the ability to monitor the status of a switch
    /// using a two-wire electrical connection between the PD and the switch.
    /// The on/off position of the switch indicates the state of an external
    /// device.
    ///
    /// The PD may simply resolve all circuit states to an open/closed
    /// status, or it may implement supervision of the monitoring circuit.
    /// A supervised circuit is able to indicate circuit fault status in
    /// addition to open/closed status.
    ContactStatusMonitoring(PdCapEntity),

    /// This function provides a switched output, typically in the form of a
    /// relay. The Output has two states: active or inactive. The Control Panel
    /// (CP) can directly set the Output's state, or, if the PD supports timed
    /// operations, the CP can specify a time period for the activation of the
    /// Output.
    OutputControl(PdCapEntity),

    /// This capability indicates the form of the card data is presented to the
    /// Control Panel.
    CardDataFormat(PdCapEntity),

    /// This capability indicates the presence of and type of LEDs.
    LedControl(PdCapEntity),

    /// This capability indicates the presence of and type of an audible
    /// annunciator (buzzer or similar tone generator).
    AudibleOutput(PdCapEntity),

    /// This capability indicates that the PD supports a text display emulating
    /// character-based display terminals.
    TextOutput(PdCapEntity),

    /// This capability indicates that the type of date and time awareness or
    /// time keeping ability of the PD.
    TimeKeeping(PdCapEntity),

    /// All PDs must be able to support the checksum mode. This capability
    /// indicates if the PD is capable of supporting CRC mode.
    CheckCharacterSupport(PdCapEntity),

    /// This capability indicates the extent to which the PD supports
    /// communication security (Secure Channel Communication)
    CommunicationSecurity(PdCapEntity),

    /// This capability indicates the maximum size single message the PD can
    /// receive.
    ReceiveBufferSize(PdCapEntity),

    /// This capability indicates the maximum size multi-part message which the
    /// PD can handle.
    LargestCombinedMessage(PdCapEntity),

    /// This capability indicates whether the PD supports the transparent mode
    /// used for communicating directly with a smart card.
    SmartCardSupport(PdCapEntity),

    /// This capability indicates the number of credential reader devices
    /// present. Compliance levels are bit fields to be assigned as needed.
    Readers(PdCapEntity),

    /// This capability indicates the ability of the reader to handle biometric
    /// input.
    Biometrics(PdCapEntity),
}

#[rustfmt::skip]
impl FromStr for PdCapability {
    type Err = OsdpError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if let Some((cap, ent)) = s.split_once(':') {
            match cap {
                "ContactStatusMonitoring" => {
                    Ok(PdCapability::ContactStatusMonitoring(PdCapEntity::from_str(ent)?))
                },
                "OutputControl" => {
                    Ok(PdCapability::OutputControl(PdCapEntity::from_str(ent)?))
                },
                "CardDataFormat" => {
                    Ok(PdCapability::CardDataFormat(PdCapEntity::from_str(ent)?))
                },
                "LedControl" => {
                    Ok(PdCapability::LedControl(PdCapEntity::from_str(ent)?))
                },
                "AudibleOutput" => {
                    Ok(PdCapability::AudibleOutput(PdCapEntity::from_str(ent)?))
                },
                "TextOutput" => {
                    Ok(PdCapability::TextOutput(PdCapEntity::from_str(ent)?))
                },
                "TimeKeeping" => {
                    Ok(PdCapability::TimeKeeping(PdCapEntity::from_str(ent)?))
                },
                "CheckCharacterSupport" => {
                    Ok(PdCapability::CheckCharacterSupport(PdCapEntity::from_str(ent)?))
                },
                "CommunicationSecurity" => {
                    Ok(PdCapability::CommunicationSecurity(PdCapEntity::from_str(ent)?))
                },
                "ReceiveBufferSize" => {
                    Ok(PdCapability::ReceiveBufferSize(PdCapEntity::from_str(ent)?))
                }
                "LargestCombinedMessage" => {
                    Ok(PdCapability::LargestCombinedMessage(PdCapEntity::from_str(ent)?))
                },
                "SmartCardSupport" => {
                    Ok(PdCapability::SmartCardSupport(PdCapEntity::from_str(ent)?))
                },
                "Readers" => {
                    Ok(PdCapability::Readers(PdCapEntity::from_str(ent)?))
                },
                "Biometrics" => {
                    Ok(PdCapability::Biometrics(PdCapEntity::from_str(ent)?))
                },
                _ => Err(OsdpError::Parse(format!("PdCapability: {s}"))),
            }
        } else {
            Err(OsdpError::Parse(format!("PdCapability: {s}")))
        }
    }
}

impl From<libosdp_sys::osdp_pd_cap> for PdCapability {
    fn from(value: libosdp_sys::osdp_pd_cap) -> Self {
        let function_code = value.function_code as u32;
        let e = PdCapEntity {
            compliance: value.compliance_level,
            num_items: value.num_items,
        };
        match function_code {
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING => {
                PdCapability::ContactStatusMonitoring(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL => {
                PdCapability::OutputControl(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT => {
                PdCapability::CardDataFormat(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL => {
                PdCapability::LedControl(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT => {
                PdCapability::AudibleOutput(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT => {
                PdCapability::TextOutput(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING => {
                PdCapability::TimeKeeping(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT => {
                PdCapability::CheckCharacterSupport(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY => {
                PdCapability::CommunicationSecurity(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE => {
                PdCapability::ReceiveBufferSize(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE => {
                PdCapability::LargestCombinedMessage(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT => {
                PdCapability::SmartCardSupport(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS => {
                PdCapability::Readers(e)
            }
            libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS => {
                PdCapability::Biometrics(e)
            }
            _ => panic!("Unknown function code"),
        }
    }
}

#[rustfmt::skip]
impl Into<u8> for PdCapability {
    fn into(self) -> u8 {
        match self {
            PdCapability::ContactStatusMonitoring(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CONTACT_STATUS_MONITORING as u8
            }
            PdCapability::OutputControl(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_OUTPUT_CONTROL as u8
            }
            PdCapability::CardDataFormat(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CARD_DATA_FORMAT as u8
            }
            PdCapability::LedControl(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_LED_CONTROL as u8
            }
            PdCapability::AudibleOutput(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT as u8
            }
            PdCapability::TextOutput(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READER_TEXT_OUTPUT as u8
            }
            PdCapability::TimeKeeping(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_TIME_KEEPING as u8
            }
            PdCapability::CheckCharacterSupport(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT as u8
            }
            PdCapability::CommunicationSecurity(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_COMMUNICATION_SECURITY as u8
            }
            PdCapability::ReceiveBufferSize(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_RECEIVE_BUFFERSIZE as u8
            }
            PdCapability::LargestCombinedMessage(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE as u8
            }
            PdCapability::SmartCardSupport(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_SMART_CARD_SUPPORT as u8
            }
            PdCapability::Readers(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_READERS as u8
            }
            PdCapability::Biometrics(_) => {
                libosdp_sys::osdp_pd_cap_function_code_e_OSDP_PD_CAP_BIOMETRICS as u8
            }
        }
    }
}

impl From<PdCapability> for libosdp_sys::osdp_pd_cap {
    fn from(value: PdCapability) -> Self {
        let function_code: u8 = value.clone().into();
        match value {
            PdCapability::ContactStatusMonitoring(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::OutputControl(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CardDataFormat(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LedControl(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::AudibleOutput(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TextOutput(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::TimeKeeping(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CheckCharacterSupport(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::CommunicationSecurity(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::ReceiveBufferSize(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::LargestCombinedMessage(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::SmartCardSupport(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Readers(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
            PdCapability::Biometrics(e) => libosdp_sys::osdp_pd_cap {
                function_code,
                compliance_level: e.compliance,
                num_items: e.num_items,
            },
        }
    }
}
