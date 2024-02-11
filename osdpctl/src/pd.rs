use crate::config::PdConfig;
use anyhow::Context;
use libosdp::{OsdpCommand, PeripheralDevice};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub fn create_pd(dev: &PdConfig) -> Result<PeripheralDevice> {
    let pd_info = dev.pd_info()
        .context("Failed to create PD info")?;
    let mut pd = PeripheralDevice::new(pd_info)?;
    pd.set_command_callback(|command| {
        match command {
            OsdpCommand::Led(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::Buzzer(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::Text(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::Output(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::ComSet(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::KeySet(c) => {
                log::info!("Command: {:?}", c);
                let mut key = [0; 16];
                key.copy_from_slice(&c.data[0..16]);
                dev.key_store.store(key).unwrap();
            }
            OsdpCommand::Mfg(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::FileTx(c) => {
                log::info!("Command: {:?}", c);
            }
            OsdpCommand::Status(c) => {
                log::info!("Command: {:?}", c);
            }
        }
        0
    });
    Ok(pd)
}
