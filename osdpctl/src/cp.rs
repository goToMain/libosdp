use crate::config::CpConfig;
use daemonize::Daemonize;
use libosdp::{OsdpEvent, ControlPanel};
use std::{fs::File, thread, time::Duration};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct CpDaemon {
    cp: ControlPanel,
}

impl CpDaemon {
    pub fn new(dev: &CpConfig) -> Result<Self> {
        let pd_info = dev.pd_info()?;
        let mut cp = ControlPanel::new(pd_info)?;
        cp.set_event_callback(|pd, event| {
            match event {
                OsdpEvent::CardRead(e) => {
                    log::info!("Event: PD-{pd} {:?}", e);
                }
                OsdpEvent::KeyPress(e) => {
                    log::info!("Event: PD-{pd} {:?}", e);
                }
                OsdpEvent::MfgReply(e) => {
                    log::info!("Event: PD-{pd} {:?}", e);
                }
                OsdpEvent::Status(e) => {
                    log::info!("Event: PD-{pd} {:?}", e);
                }
            }
            0
        });
        let stdout = File::create(
            dev.log_dir
                .as_path()
                .join(format!("pd-{}.out.log", &dev.name).as_str()),
        )?;
        let stderr = File::create(
            dev.log_dir
                .as_path()
                .join(format!("pd-{}.err.log", &dev.name).as_str()),
        )?;
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
