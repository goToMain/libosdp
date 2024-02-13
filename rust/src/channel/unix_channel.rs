//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

//! OSDP unix channel

use super::Channel;
use core::time::Duration;
use std::{
    io::{Read, Write},
    os::unix::net::{UnixListener, UnixStream},
    path::{Path, PathBuf},
    str::FromStr,
    thread,
};

type Result<T> = std::result::Result<T, crate::OsdpError>;

/// A reference OSDP channel implementation for unix domain socket.
#[derive(Debug)]
pub struct UnixChannel {
    id: i32,
    stream: UnixStream,
}

impl UnixChannel {
    /// Connect to a channel identified by `name`.
    pub fn connect(path: &Path) -> Result<Self> {
        let id = super::str_to_channel_id(path.as_os_str().try_into().unwrap());
        let stream = UnixStream::connect(&path)?;
        Ok(Self { id, stream })
    }

    /// Listen on a channel identified by `name`.
    pub fn new(path: &Path) -> Result<Self> {
        let id = super::str_to_channel_id(path.as_os_str().try_into().unwrap());
        if path.exists() {
            std::fs::remove_file(&path)?;
        }
        let listener = UnixListener::bind(&path)?;
        println!("Waiting for connection to unix::{}", path.display());
        let (stream, _) = listener.accept()?;
        Ok(Self { id, stream })
    }

    /// Create a bi-directional channel pair. Returns Result<(server, client)>
    pub fn pair(name: &str) -> Result<(Self, Self)> {
        let path = PathBuf::from_str(format!("/tmp/osdp-{name}.sock").as_str())?;
        let path_clone = path.clone();
        let h = thread::spawn(move || {
            let path = path_clone;
            UnixChannel::new(&path)
        });
        thread::sleep(Duration::from_secs(1));
        let client = UnixChannel::connect(&path)?;
        let server = h.join().unwrap()?;
        Ok((server, client))
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
