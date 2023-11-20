pub mod unix_channel;

use crate::osdp_sys;
use lazy_static::lazy_static;
use std::{
    collections::{hash_map::DefaultHasher, HashMap},
    ffi::c_void,
    hash::{Hash, Hasher},
    io::{Read, Write},
    sync::{Mutex, Arc},
};

lazy_static! {
    static ref CHANNELS: Mutex<HashMap<i32, Arc<Mutex<Box<dyn Channel>>>>> = Mutex::new(HashMap::new());
}

impl std::fmt::Debug for OsdpChannel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("OsdpChannel")
            .field(
                "stream",
                &format!("{}", self.stream.lock().unwrap().get_id()))
            .finish()
    }
}

unsafe extern "C" fn raw_read(data: *mut c_void, buf: *mut u8, len: i32) -> i32 {
    let key = data as i32;
    let mut channels = CHANNELS.lock().unwrap();
    let mut stream = channels.get_mut(&key).unwrap().lock().unwrap();
    let mut read_buf = vec![0u8; len as usize];
    match stream.read(&mut read_buf) {
        Ok(n) => {
            let src_ptr = read_buf.as_mut_ptr();
            std::ptr::copy_nonoverlapping(src_ptr, buf, len as usize);
            n as i32
        }
        Err(_) => -1,
    }
}

unsafe extern "C" fn raw_write(data: *mut c_void, buf: *mut u8, len: i32) -> i32 {
    let key = data as i32;
    let mut channels = CHANNELS.lock().unwrap();
    let mut stream = channels.get_mut(&key).unwrap().lock().unwrap();
    let mut write_buf = vec![0u8; len as usize];
    std::ptr::copy_nonoverlapping(buf, write_buf.as_mut_ptr(), len as usize);
    match stream.write(&write_buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

unsafe extern "C" fn raw_flush(data: *mut c_void) {
    let key = data as i32;
    let mut channels = CHANNELS.lock().unwrap();
    let mut stream = channels.get_mut(&key).unwrap().lock().unwrap();
    let _ = stream.flush();
}

pub trait Channel: Read + Write + Sync + Send {
    fn get_id(&self) -> i32;
}

pub struct OsdpChannel {
    stream: Arc<Mutex<Box<dyn Channel>>>,
}

impl OsdpChannel {
    pub fn new<T: Channel>(stream: Box<dyn Channel>) -> OsdpChannel {
        Self { stream: Arc::new(Mutex::new(stream)) }
    }

    pub fn as_struct(&self) -> osdp_sys::osdp_channel {
        let stream = self.stream.clone();
        let id = stream.lock().unwrap().get_id();
        CHANNELS.lock().unwrap().insert(id, stream);
        osdp_sys::osdp_channel {
            id,
            data: id as *mut c_void,
            recv: Some(raw_read),
            send: Some(raw_write),
            flush: Some(raw_flush),
        }
    }
}

pub fn str_to_channel_id(key: &str) -> i32 {
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let mut id = hasher.finish();
    id = (id >> 32) ^ id & 0xffffffff;
    id as i32
}
