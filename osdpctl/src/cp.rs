use crate::config::CpConfig;
use anyhow::Context;
use libosdp::{OsdpEvent, ControlPanel};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub fn create_cp(dev: &CpConfig) -> Result<ControlPanel> {
    let pd_info = dev.pd_info()
        .context("Failed to create PD info list")?;
    let mut cp = ControlPanel::new(pd_info)?;
    cp.set_event_callback(|pd, event| {
        match event {
            OsdpEvent::CardRead(e) => {
                log::info!("Event: PD-{pd} {:?}", e);
            }
            OsdpEvent::KeyPress(e) => {
                log::info!("Event: PD-{pd} {:?}", e);
            }
            OsdpEvent::MfgReply(e) => {
                log::info!("Event: PD-{pd} {:?}", e);
            }
            OsdpEvent::Status(e) => {
                log::info!("Event: PD-{pd} {:?}", e);
            }
        }
        0
    });
    Ok(cp)
}
