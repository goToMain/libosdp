//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

use std::{thread, time::Duration};

use crate::config::CpConfig;
use anyhow::Context;
use libosdp::{ControlPanel, OsdpEvent};
use std::io::Write;

type Result<T> = anyhow::Result<T, anyhow::Error>;

fn setup(dev: &CpConfig, daemonize: bool) -> Result<()> {
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

pub fn main(dev: CpConfig, daemonize: bool) -> Result<()> {
    setup(&dev, daemonize)?;
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
    loop {
        cp.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}
