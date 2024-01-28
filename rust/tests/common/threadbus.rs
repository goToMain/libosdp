use multiqueue::{BroadcastReceiver, BroadcastSender};
use std::{
    collections::hash_map::DefaultHasher,
    fmt::Debug,
    hash::{Hash, Hasher},
    io::Error,
    io::ErrorKind,
    sync::Mutex,
};

fn str_to_channel_id(key: &str) -> i32 {
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let mut id: u64 = hasher.finish();
    id = (id >> 32) ^ id & 0xffffffff;
    id as i32
}

pub struct ThreadBus {
    name: String,
    id: i32,
    send: Mutex<BroadcastSender<Vec<u8>>>,
    recv: Mutex<BroadcastReceiver<Vec<u8>>>,
}

impl ThreadBus {
    pub fn new(name: &str) -> Self {
        let (send, recv) = multiqueue::broadcast_queue(4);
        Self {
            name: name.into(),
            id: str_to_channel_id(name),
            send: Mutex::new(send),
            recv: Mutex::new(recv),
        }
    }
}

impl Clone for ThreadBus {
    fn clone(&self) -> Self {
        let send = Mutex::new(self.send.lock().unwrap().clone());
        let recv = Mutex::new(self.recv.lock().unwrap().add_stream());
        Self {
            name: self.name.clone(),
            id: self.id,
            send,
            recv,
        }
    }
}

impl Debug for ThreadBus {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ThreadBus")
            .field("name", &self.name)
            .field("id", &self.id)
            .finish()
    }
}

impl std::io::Read for ThreadBus {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        let v = self.recv.lock().unwrap().try_recv().map_err(|e| match e {
            std::sync::mpsc::TryRecvError::Empty => Error::new(ErrorKind::WouldBlock, "No data"),
            std::sync::mpsc::TryRecvError::Disconnected => {
                Error::new(ErrorKind::ConnectionReset, "disconnected")
            }
        })?;
        buf[..v.len()].copy_from_slice(&v[..]);
        Ok(v.len())
    }
}

impl std::io::Write for ThreadBus {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        let v = buf.into();
        self.send.lock().unwrap().try_send(v).map_err(|e| match e {
            std::sync::mpsc::TrySendError::Full(_) => Error::new(ErrorKind::WouldBlock, "No space"),
            std::sync::mpsc::TrySendError::Disconnected(_) => {
                Error::new(ErrorKind::ConnectionReset, "disconnected")
            }
        })?;
        Ok(buf.len())
    }

    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

impl libosdp::channel::Channel for ThreadBus {
    fn get_id(&self) -> i32 {
        self.id
    }
}
