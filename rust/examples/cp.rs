use std::{
    io::{Read,Write},
    path::PathBuf,
    os::unix::net::UnixStream,
    time::Duration,
    thread
};
use osdp::{
    cp::ControlPanel,
    common::{PdInfo, OsdpFlag, PdId},
    channel::{OsdpChannel, Channel}
};

type Result<T> = anyhow::Result<T, anyhow::Error>;

struct UnixChannel {
    stream: UnixStream,
}

impl UnixChannel {
    pub fn new(path: PathBuf) -> Result<Self> {
        let stream = UnixStream::connect(&path)?;
        Ok(Self {
            stream,
        })
    }
}

impl Read for UnixChannel {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        self.stream.read(buf)
    }
}

impl Write for UnixChannel {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.stream.write(buf)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.stream.flush()
    }
}

impl Channel for UnixChannel {
    fn get_id(&self) -> i32 {
        1
    }
}

fn main() -> anyhow::Result<(), anyhow::Error> {
    let stream = UnixChannel::new("/tmp/osdp-conn-1".into())?;
    let channel: OsdpChannel = OsdpChannel::new::<UnixChannel>(Box::new(stream));
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
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]
    );
    let mut pd_info = vec![pd0];
    let mut cp = ControlPanel::new(&mut pd_info)?;
    loop {
        cp.refresh();
        thread::sleep(Duration::from_millis(50));
    }
}