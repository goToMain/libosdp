use crate::config::PdConfig;
use daemonize::Daemonize;
use libosdp::{OsdpCommand, PeripheralDevice};
use std::{fs::File, thread, time::Duration};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct PdDaemon {
    pd: PeripheralDevice,
}

impl PdDaemon {
    pub fn new(dev: &PdConfig) -> Result<Self> {
        let pd_info = dev.pd_info()?;
        let mut pd = PeripheralDevice::new(pd_info)?;
        pd.set_command_callback(|command| {
            match command {
                OsdpCommand::Led(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::Buzzer(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::Text(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::Output(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::ComSet(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::KeySet(c) => {
                    log::info!("Command: {:?}", c);
                    let mut key = [0; 16];
                    key.copy_from_slice(&c.data[0..16]);
                    dev.key_store.store(key).unwrap();
                }
                OsdpCommand::Mfg(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::FileTx(c) => {
                    log::info!("Command: {:?}", c);
                }
                OsdpCommand::Status(c) => {
                    log::info!("Command: {:?}", c);
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
            Ok(_) => println!("Success, demonized"),
            Err(e) => eprintln!("Error, {}", e),
        }
        Ok(Self { pd })
    }

    pub fn run(&mut self) {
        loop {
            self.pd.refresh();
            thread::sleep(Duration::from_millis(50));
        }
    }
}
