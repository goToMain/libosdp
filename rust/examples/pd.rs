use std::{
    time::Duration,
    thread
};
use osdp::{
    pd::PeripheralDevice,
    common::{PdInfo, OsdpFlag, PdId, PdCapability, PdCapEntry},
    channel::{OsdpChannel, unix_channel::UnixChannel},
};
fn main() -> anyhow::Result<()> {
    let stream = UnixChannel::new("conn-1")?;
    let channel: OsdpChannel = OsdpChannel::new::<UnixChannel>(Box::new(stream));
    let mut pd_info =  PdInfo::new(
        "PD 101",
        101,
        115200,
        OsdpFlag::EnforceSecure,
        PdId {
            version: 0x01,
            model: 0x02,
            vendor_code: 0x03,
            serial_number: 0x04,
            firmware_version: 0x05,
        },
        vec![
            PdCapability::CommunicationSecurity(PdCapEntry { compliance: 1, num_items: 1 }),
        ],
        channel,
        [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]
    );
    let mut pd = PeripheralDevice::new(&mut pd_info)?;
    pd.set_command_callback(|_| {
        println!("Received command!");
        0
    });
    loop {
        pd.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}