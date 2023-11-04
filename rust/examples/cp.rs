use std::{
    time::Duration,
    thread
};
use libosdp::{
    cp::ControlPanel,
    common::{PdInfo, OsdpFlag, PdId},
    channel::{OsdpChannel, unix_channel::UnixChannel},
};

fn main() -> anyhow::Result<()> {
    env_logger::init();
    let stream = UnixChannel::connect("conn-1")?;
    let mut pd_info = vec![
        PdInfo::new(
            "PD 101", 101,
            115200,
            OsdpFlag::EnforceSecure,
            PdId {
                version: 0x01,
                model: 0x02,
                vendor_code: 0x03,
                serial_number: 0x04,
                firmware_version: 0x05,
            },
            vec![],
            OsdpChannel::new::<UnixChannel>(Box::new(stream)),
            [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
            ]
        ),
    ];
    let mut cp = ControlPanel::new(&mut pd_info)?;
    loop {
        cp.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}