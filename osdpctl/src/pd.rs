//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

use std::{thread, time::Duration};

use crate::config::PdConfig;
use anyhow::Context;
use libosdp::{OsdpCommand, PeripheralDevice};
use std::io::Write;

type Result<T> = anyhow::Result<T, anyhow::Error>;

fn setup(dev: &PdConfig, daemonize: bool) -> Result<()> {
    if dev.runtime_dir.exists() {
        std::fs::remove_dir_all(&dev.runtime_dir)?;
    }
    std::fs::create_dir_all(&dev.runtime_dir)?;
    if daemonize {
        crate::daemonize::daemonize(&dev.runtime_dir, &dev.name)?;
    } else {
        let pid_file = dev.runtime_dir.join(format!("dev-{}.pid", dev.name));
        let mut pid_file = std::fs::File::create(pid_file)?;
        write!(pid_file, "{}", std::process::id())?;
    }
    Ok(())
}

pub fn main(dev: PdConfig, daemonize: bool) -> Result<()> {
    setup(&dev, daemonize)?;
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
    loop {
        pd.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}
