use anyhow::bail;
use multiqueue::{BroadcastReceiver, BroadcastSender};
use std::{collections::HashMap, io::Error, io::ErrorKind, sync::Mutex};

use super::Channel;

type Result<T> = anyhow::Result<T, anyhow::Error>;

struct Bus {
    id: i32,
    ref_count: usize,
    send: BroadcastSender<Vec<u8>>,
    recv: BroadcastReceiver<Vec<u8>>,
}

impl Bus {
    pub fn new(id: i32) -> Self {
        let (send, recv) = multiqueue::broadcast_queue(5);
        Self {
            id,
            ref_count: 0,
            send,
            recv,
        }
    }

    pub fn get(&mut self) -> (i32, BroadcastSender<Vec<u8>>, BroadcastReceiver<Vec<u8>>) {
        self.ref_count += 1;
        (self.id, self.send.clone(), self.recv.add_stream())
    }

    pub fn put(&mut self) {
        self.ref_count -= 1;
    }
}

struct BusDepot {
    id_max: i32,
    bus_map: HashMap<String, Bus>,
}

impl BusDepot {
    pub fn new() -> Self {
        Self {
            id_max: 0,
            bus_map: HashMap::new(),
        }
    }

    pub fn get(
        &mut self,
        name: &str,
    ) -> (i32, BroadcastSender<Vec<u8>>, BroadcastReceiver<Vec<u8>>) {
        if self.bus_map.get(name).is_none() {
            self.id_max += 1;
            let bus = Bus::new(self.id_max);
            self.bus_map.insert(name.into(), bus);
        }
        self.bus_map.get_mut(name).unwrap().get()
    }

    pub fn put(&mut self, name: &str) -> Result<()> {
        if let Some(bus) = self.bus_map.get_mut(name) {
            bus.put();
        } else {
            bail!("Key does not exist");
        }
        Ok(())
    }
}

lazy_static::lazy_static! {
    static ref ID: Mutex<BusDepot> = Mutex::new(BusDepot::new());
}

pub struct OsdpThreadBus {
    name: String,
    id: i32,
    send: BroadcastSender<Vec<u8>>,
    recv: BroadcastReceiver<Vec<u8>>,
}

impl std::fmt::Debug for OsdpThreadBus {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("OsdpThreadBus")
            .field("name", &self.name)
            .field("id", &self.id)
            .field("send", &format!("BroadcastSender::<Vec<u8>>"))
            .field("recv", &self.recv)
        .finish()
    }
}

impl OsdpThreadBus {
    pub fn join(name: &str) -> Self {
        let (id, send, recv) = ID.lock().unwrap().get(name);
        Self {
            name: name.into(),
            id,
            send,
            recv,
        }
    }
}

impl std::io::Read for OsdpThreadBus {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        let v = self
            .recv
            .recv()
            .map_err(|_| Error::new(ErrorKind::Other, "Bus recv error!"))?;
        buf[..v.len()].copy_from_slice(&v[..]);
        Ok(v.len())
    }
}

impl std::io::Write for OsdpThreadBus {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        let v = buf.into();
        self.send
            .try_send(v)
            .map_err(|_| Error::new(ErrorKind::Other, "Bus send error!"))?;
        _ = self.recv.recv(); // get rid of local echo
        Ok(buf.len())
    }

    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

impl Channel for OsdpThreadBus {
    fn get_id(&self) -> i32 {
        self.id
    }
}

impl Drop for OsdpThreadBus {
    fn drop(&mut self) {
        ID.lock().unwrap().put(&self.name).unwrap();
    }
}
