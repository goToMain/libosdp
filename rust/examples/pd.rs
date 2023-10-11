use osdp::{
    pd::PeripheralDevice,
    common::{PdInfo, OsdpFlag, PdId},
    channel::OsdpChannel, events::OsdpEventCardRead,
};

fn main() -> anyhow::Result<(), anyhow::Error> {
    let stream = todo!();
    let channel = OsdpChannel::new(Box::new(stream));
    let pd_info =  PdInfo::new(
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
        channel,
        [
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        ]
    );
    let pd = PeripheralDevice::new(&mut pd_info)?;
    pd.set_command_callback(|cmd| {
        println!("Received command!");
        0
    });
    loop {
        pd.refresh();
    }
    Ok(())
}