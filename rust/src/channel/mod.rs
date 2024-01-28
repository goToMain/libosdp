//! The OSDP specification defines that communication between OSDP devices
//! happen over an RS-485 connection. For testing and development purpose this
//! can be limiting so LibOSDP defines a notion called "Channel" which is a
//! software representation (abstraction) of the physical transport medium.
//!
//! Since RS-485 is stream based protocol, we can think of it to be  something
//! that we can read from and write to (which in turn is Read and Write traits
//! in rust). This allows us to run OSDP devices over many IPC schemes such as
//! Unix socket and message queues.
//!
//! This module provides a way to define an OSDP channel and export it to
//! LibOSDP.

mod memory_channel;
#[cfg(feature = "std")]
mod unix_channel;

#[cfg(feature = "std")]
pub use memory_channel::MemoryChannel;
use once_cell::sync::Lazy;
#[cfg(feature = "std")]
pub use unix_channel::UnixChannel;

#[cfg(not(feature = "std"))]
use alloc::collections::BTreeMap as HashMap;
use alloc::{boxed::Box, format, sync::Arc, vec};
use core::ffi::c_void;
#[cfg(feature = "std")]
use std::{
    collections::{hash_map::DefaultHasher, HashMap},
    hash::{Hash, Hasher},
};

#[cfg(feature = "std")]
use parking_lot::Mutex;
#[cfg(not(feature = "std"))]
use spin::Mutex;

static CHANNELS: Lazy<Mutex<HashMap<i32, Arc<Mutex<Box<dyn Channel>>>>>> =
    Lazy::new(|| Mutex::new(HashMap::new()));

impl alloc::fmt::Debug for OsdpChannel {
    fn fmt(&self, f: &mut alloc::fmt::Formatter<'_>) -> alloc::fmt::Result {
        f.debug_struct("OsdpChannel")
            .field("stream", &format!("{}", self.stream.lock().get_id()))
            .finish()
    }
}

unsafe extern "C" fn raw_read(data: *mut c_void, buf: *mut u8, len: i32) -> i32 {
    let key = data as i32;
    let mut channels = CHANNELS.lock();
    let mut stream = channels.get_mut(&key).unwrap().lock();
    let mut read_buf = vec![0u8; len as usize];
    match stream.read(&mut read_buf) {
        Ok(n) => {
            let src_ptr = read_buf.as_mut_ptr();
            core::ptr::copy_nonoverlapping(src_ptr, buf, len as usize);
            n as i32
        }
        Err(_) => -1,
    }
}

unsafe extern "C" fn raw_write(data: *mut c_void, buf: *mut u8, len: i32) -> i32 {
    let key = data as i32;
    let mut channels = CHANNELS.lock();
    let mut stream = channels.get_mut(&key).unwrap().lock();
    let mut write_buf = vec![0u8; len as usize];
    core::ptr::copy_nonoverlapping(buf, write_buf.as_mut_ptr(), len as usize);
    match stream.write(&write_buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

unsafe extern "C" fn raw_flush(data: *mut c_void) {
    let key = data as i32;
    let mut channels = CHANNELS.lock();
    let mut stream = channels.get_mut(&key).unwrap().lock();
    let _ = stream.flush();
}

#[cfg(not(feature = "std"))]
/// The error type for I/O operations of the [`Read`] and [`Write`] associated traits.
pub type RWError = Box<dyn embedded_io::Error>;
#[cfg(feature = "std")]
/// The error type for I/O operations of the [`Read`] and [`Write`] associated traits.
pub type RWError = std::io::Error;

/// The `Read` trait allows for reading bytes from a source.
///
/// Wrapper around either [`std::io::Read`] or [`embedded_io::Read`] depending on the availability of `std`.
pub trait Read {
    /// Pull some bytes from this source into the specified buffer, returning
    /// how many bytes were read.
    ///
    /// Wrapper around either [`std::io::Read::read`] or [`embedded_io::Read::read`] depending on the availability of `std`.
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, RWError>;
}

/// A trait for objects which are byte-oriented sinks.
///
/// Wrapper around either [`std::io::Write`] or [`embedded_io::Write`] depending on the availability of `std`.
pub trait Write {
    /// Write a buffer into this writer, returning how many bytes were written.
    ///
    /// Wrapper around either [`std::io::Write::write`] or [`embedded_io::Write::write`] depending on the availability of `std`.
    fn write(&mut self, buf: &[u8]) -> Result<usize, RWError>;
    /// Flush this output stream, ensuring that all intermediately buffered
    /// contents reach their destination.
    ///
    /// Wrapper around either [`std::io::Write::flush`] or [`embedded_io::Write::flush`] depending on the availability of `std`.
    fn flush(&mut self) -> Result<(), RWError>;
}

#[cfg(feature = "std")]
impl<T: std::io::Read> Read for T {
    #[inline(always)]
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, RWError> {
        self.read(buf)
    }
}

#[cfg(feature = "std")]
impl<T: std::io::Write> Write for T {
    #[inline(always)]
    fn write(&mut self, buf: &[u8]) -> Result<usize, RWError> {
        self.write(buf)
    }
    #[inline(always)]
    fn flush(&mut self) -> Result<(), RWError> {
        self.flush()
    }
}

#[cfg(not(feature = "std"))]
impl<T: embedded_io::Read> Read for T
where
    T::Error: 'static,
{
    #[inline(always)]
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, RWError> {
        self.read(buf)
            .map_err(|e| Box::new(e) as Box<dyn embedded_io::Error>)
    }
}

#[cfg(not(feature = "std"))]
impl<T: embedded_io::Write> Write for T
where
    T::Error: 'static,
{
    #[inline(always)]
    fn write(&mut self, buf: &[u8]) -> Result<usize, RWError> {
        self.write(buf)
            .map_err(|e| Box::new(e) as Box<dyn embedded_io::Error>)
    }
    #[inline(always)]
    fn flush(&mut self) -> Result<(), RWError> {
        self.flush()
            .map_err(|e| Box::new(e) as Box<dyn embedded_io::Error>)
    }
}

/// The Channel trait acts as an interface for all channel implementors. See
/// module description for the definition of a "channel" in LibOSDP.
pub trait Channel: Read + Write + Send + Sync {
    /// Since OSDP channels can be multi-drop (more than one PD can talk to a
    /// CP on the same channel) and LibOSDP supports mixing multi-drop
    /// connections among PDs it needs a way to identify each unique channel by
    /// a channel ID. Implementors of this trait must also provide a method
    /// which returns a unique i32 per channel.
    fn get_id(&self) -> i32;
}

/// A wrapper to hold anything that implements the Channel trait. It is used to
/// export the inner value to LibOSDP in a format that it expects it to be in.
pub struct OsdpChannel {
    stream: Arc<Mutex<Box<dyn Channel>>>,
}

impl OsdpChannel {
    /// Create an instance of OsdpChannel for a given object that implements
    /// the Channel Trait.
    pub fn new<T: Channel>(stream: Box<dyn Channel>) -> OsdpChannel {
        Self {
            stream: Arc::new(Mutex::new(stream)),
        }
    }

    /// For internal use; in as_struct() of [`crate::PdInfo`]. This methods
    /// exports the channel to LibOSDP as a C struct.
    pub fn as_struct(&self) -> libosdp_sys::osdp_channel {
        let stream = self.stream.clone();
        let id = stream.lock().get_id();
        CHANNELS.lock().insert(id, stream);
        libosdp_sys::osdp_channel {
            id,
            data: id as *mut c_void,
            recv: Some(raw_read),
            send: Some(raw_write),
            flush: Some(raw_flush),
        }
    }
}

#[cfg(feature = "std")]
fn str_to_channel_id(key: &str) -> i32 {
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let mut id = hasher.finish();
    id = (id >> 32) ^ id & 0xffffffff;
    id as i32
}
