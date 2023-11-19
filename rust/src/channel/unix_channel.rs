use super::Channel;
use std::{
    io::{Read, Write},
    os::unix::net::{UnixListener, UnixStream},
    path::PathBuf,
    str::FromStr,
};

type Result<T> = anyhow::Result<T, anyhow::Error>;

#[derive(Debug)]
pub struct UnixChannel {
    id: i32,
    stream: UnixStream,
}

impl UnixChannel {
    pub fn connect(name: &str) -> Result<Self> {
        let path = format!("/tmp/osdp-{name}");
        let id = super::str_to_channel_id(&path);
        let stream = UnixStream::connect(&path)?;
        Ok(Self { id, stream })
    }

    pub fn new(name: &str) -> Result<Self> {
        let path = format!("/tmp/osdp-{name}");
        let id = super::str_to_channel_id(&path);
        let path = PathBuf::from_str(&path)?;
        if path.exists() {
            std::fs::remove_file(&path)?;
        }
        let listener = UnixListener::bind(&path)?;
        println!("Waiting for connection to unix::{}", path.display());
        let (stream, _) = listener.accept()?;
        Ok(Self { id, stream })
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
        self.id
    }
}
