use std::{fs::File, thread, time::Duration};
use daemonize::Daemonize;
use libosdp::{
    cp::ControlPanel, events::OsdpEvent
};
use crate::config::CpConfig;

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct CpDaemon {
    cp: ControlPanel,
}

impl CpDaemon {
    pub fn new(dev: &CpConfig) -> Result<Self> {
        let mut pd_info = dev.pd_info()?;
        let mut cp = ControlPanel::new(&mut pd_info)?;
        cp.set_event_callback(|pd, event| {
            match event {
                OsdpEvent::CardRead(e) => {
                    println!("Event: PD-{pd} {:?}", e);
                },
                OsdpEvent::KeyPress(e) => {
                    println!("Event: PD-{pd} {:?}", e);
                },
                OsdpEvent::MfgReply(e) => {
                    println!("Event: PD-{pd} {:?}", e);
                },
                OsdpEvent::IO(e) => {
                    println!("Event: PD-{pd} {:?}", e);
                },
                OsdpEvent::Status(e) => {
                    println!("Event: PD-{pd} {:?}", e);
                },
            }
            0
        });
        let stdout = File::create("/tmp/daemon.out").unwrap();
        let stderr = File::create("/tmp/daemon.err").unwrap();
        let daemon = Daemonize::new()
            .pid_file(&dev.pid_file)
            .chown_pid_file(true)
            .working_directory("/tmp")
            .stdout(stdout)
            .stderr(stderr);
        match daemon.start() {
            Ok(_) => println!("Success, daemonize"),
            Err(e) => eprintln!("Error, {}", e),
        }
        Ok(Self { cp })
    }

    pub fn run(&mut self) {
        loop {
            self.cp.refresh();
            thread::sleep(Duration::from_millis(50));
        }
    }
}