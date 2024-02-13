//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

use libosdp::{
    channel::{OsdpChannel, UnixChannel},
    OsdpError, OsdpFlag, PdCapEntity, PdCapability, PdId, PdInfo, PeripheralDevice,
};
use std::{result::Result, thread, time::Duration, path::PathBuf, str::FromStr};

fn main() -> Result<(), OsdpError> {
    env_logger::builder()
        .filter_level(log::LevelFilter::Info)
        .format_target(false)
        .format_timestamp(None)
        .init();
    let path = PathBuf::from_str("/tmp/conn-1")?;
    let stream = UnixChannel::new(&path)?;
    let pd_info = PdInfo::for_pd(
        "PD 101",
        101,
        115200,
        OsdpFlag::EnforceSecure,
        PdId::from_number(101),
        vec![PdCapability::CommunicationSecurity(PdCapEntity::new(1, 1))],
        OsdpChannel::new::<UnixChannel>(Box::new(stream)),
        [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
            0x0e, 0x0f,
        ],
    );
    let mut pd = PeripheralDevice::new(pd_info)?;
    pd.set_command_callback(|_| {
        println!("Received command!");
        0
    });
    loop {
        pd.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}
