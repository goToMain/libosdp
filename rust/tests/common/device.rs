use std::{
    sync::{mpsc::Receiver, Arc, Mutex, MutexGuard},
    thread, time,
};

use libosdp::{
    channel::{Channel, OsdpChannel},
    ControlPanel, OsdpCommand, OsdpEvent, OsdpFlag, PdCapEntity, PdCapability, PdId, PdInfo,
    PeripheralDevice,
};
type Result<T> = core::result::Result<T, libosdp::OsdpError>;

pub struct CpDevice {
    dev: Arc<Mutex<ControlPanel>>,
    pub receiver: Receiver<(i32, OsdpEvent)>,
}

impl CpDevice {
    pub fn new<T: Channel + 'static>(bus: T) -> Result<Self> {
        let pd_info = vec![PdInfo::for_cp(
            "PD 101",
            101,
            115200,
            OsdpFlag::empty(),
            OsdpChannel::new::<T>(Box::new(bus)),
            [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f,
            ],
        )];
        let mut cp = ControlPanel::new(pd_info)?;
        let (event_tx, event_rx) = std::sync::mpsc::channel::<(i32, OsdpEvent)>();

        cp.set_event_callback(|pd, event| {
            event_tx.send((pd, event)).unwrap();
            0
        });

        let dev = Arc::new(Mutex::new(cp));
        let dev_clone = dev.clone();
        let _ = thread::Builder::new()
            .name("CP Thread".to_string())
            .spawn(move || {
                let dev = dev_clone;
                let sender = event_tx;
                dev.lock().unwrap().set_event_callback(|pd, event| {
                    sender.send((pd, event)).expect("CP event send");
                    0
                });
                loop {
                    dev.lock().unwrap().refresh();
                    thread::sleep(time::Duration::from_millis(10));
                }
            });
        Ok(Self {
            dev,
            receiver: event_rx,
        })
    }

    pub fn get_device(&self) -> MutexGuard<'_, ControlPanel> {
        self.dev.lock().unwrap()
    }
}

pub struct PdDevice {
    dev: Arc<Mutex<PeripheralDevice>>,
    pub receiver: Receiver<OsdpCommand>,
}

impl PdDevice {
    pub fn new<T: Channel + 'static>(bus: T) -> Result<Self> {
        let pd_info = PdInfo::for_pd(
            "PD 101",
            101,
            115200,
            OsdpFlag::empty(),
            PdId::from_number(101),
            vec![
                PdCapability::CommunicationSecurity(PdCapEntity::new(1, 1)),
                PdCapability::AudibleOutput(PdCapEntity::new(1, 1)),
                PdCapability::LedControl(PdCapEntity::new(1, 1)),
            ],
            OsdpChannel::new::<T>(Box::new(bus)),
            [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f,
            ],
        );
        let mut pd = PeripheralDevice::new(pd_info)?;
        let (cmd_tx, cmd_rx) = std::sync::mpsc::channel::<OsdpCommand>();
        pd.set_command_callback(|command| {
            cmd_tx.send(command).unwrap();
            0
        });

        let dev = Arc::new(Mutex::new(pd));
        let dev_clone = dev.clone();
        let _ = thread::Builder::new()
            .name("PD Thread".to_string())
            .spawn(move || {
                let dev = dev_clone;
                let sender = cmd_tx;
                dev.lock().unwrap().set_command_callback(|command| {
                    println!("--> Got here! {:?}", command);
                    sender.send(command).expect("PD command send");
                    println!("<-- Got out!");
                    0
                });
                loop {
                    dev.lock().unwrap().refresh();
                    thread::sleep(time::Duration::from_millis(10));
                }
            });

        Ok(Self {
            dev,
            receiver: cmd_rx,
        })
    }

    pub fn get_device(&self) -> MutexGuard<'_, PeripheralDevice> {
        self.dev.lock().unwrap()
    }
}
