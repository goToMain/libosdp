use osdp::{
    cp::ControlPanel,
    common::{PdInfo, OsdpFlag, PdId},
    channel::OsdpChannel
};

fn main() -> anyhow::Result<(), anyhow::Error> {
    let stream = todo!();
    let channel = OsdpChannel::new(Box::new(stream));
    let pd0 =  PdInfo::new(
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
    let pd_info = vec![pd0];
    let cp = ControlPanel::new(&mut pd_info)?;
    loop {
        cp.refresh();
    }
    Ok(())
}