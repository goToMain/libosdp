use std::{thread, time::Duration, fs::File};
use daemonize::Daemonize;
use libosdp::{
    pd::PeripheralDevice,
    commands::OsdpCommand
};
use crate::config::PdConfig;

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct PdDaemon {
    pd: PeripheralDevice,
}

impl PdDaemon {
    pub fn new(dev: &PdConfig) -> Result<Self> {
        let mut pd_info = dev.pd_info()?;
        let mut pd = PeripheralDevice::new(&mut pd_info)?;
        pd.set_command_callback(|command| {
            match command {
                OsdpCommand::Led(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::Buzzer(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::Text(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::Output(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::ComSet(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::KeySet(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::Mfg(c) => {
                    println!("Command: {:?}", c);
                },
                OsdpCommand::FileTx(c) => {
                    println!("Command: {:?}", c);
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
        Ok(Self { pd })
    }

    pub fn run(&mut self) {
        loop {
            self.pd.refresh();
            thread::sleep(Duration::from_millis(50));
        }
    }
}