mod config;
mod cp;
mod pd;

use std::{
    path::PathBuf,
    collections::HashMap,
};

use clap::{arg, Command};
use config::{AppConfig, DeviceConfig};
use cp::CpDaemon;
use pd::PdDaemon;

type Result<T> = anyhow::Result<T, anyhow::Error>;

fn cli() -> Command {
    Command::new("osdpctl")
        .about("Setup and manage OSDP devices")
        .subcommand_required(true)
        .arg_required_else_help(true)
        .allow_external_subcommands(true)
        .subcommand(
            Command::new("list")
                .about("List configured OSDP devices")
        )
        .subcommand(
            Command::new("start")
                .about("Start a OSDP device")
                .arg(arg!(<DEV> "device to start"))
                .arg_required_else_help(true),
        )
        .subcommand(
            Command::new("stop")
                .about("Stop a running OSDP device")
                .arg(arg!(<DEV> "device to stop"))
                .arg_required_else_help(true),
        )
        .subcommand(
            Command::new("attach")
                .about("Stop a running OSDP device")
                .arg(arg!(<DEV> "device device to attach to"))
                .arg_required_else_help(true),
        )
}

fn osdpctl_config_dir() -> Result<PathBuf> {
    let mut cfg_dir;
    if let Ok(dir) = std::env::var("OSDPCTL_CONF_DIR") {
        cfg_dir = std::fs::canonicalize(&dir)?;
    } else {
        cfg_dir = dirs::config_dir()
            .or_else(|| Some(PathBuf::from("/tmp/.config/"))).unwrap();
        cfg_dir.push("osdp");
        if !cfg_dir.is_dir() {
            std::fs::create_dir_all(&cfg_dir)?;
        }
    }
    Ok(cfg_dir)
}

fn main() -> Result<()> {
    let cfg_dir = osdpctl_config_dir()?;
    let config = AppConfig::from(&cfg_dir.join("osdpctl.cfg"))?;

    let paths = std::fs::read_dir(&config.device_config_dir).unwrap();
    let mut device_map: HashMap<String, DeviceConfig> = HashMap::new();
    for path in paths {
        let path = path.unwrap().path();
        if let Some(ext) = path.extension() {
            if ext == "cfg" {
                let dev = DeviceConfig::from_path(&path)?;
                device_map.insert(dev.name().to_owned(), dev);
            }
        }
    }

    match cli().get_matches().subcommand() {
        Some(("list", _)) => {
            for (name, _) in device_map {
                println!("{}: online", name);
            }
        }
        Some(("start", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                match dev {
                    DeviceConfig::CpConfig(c) => {
                        let mut daemon = CpDaemon::new(c)?;
                        daemon.run();
                    },
                    DeviceConfig::PdConfig(c) => {
                        let mut daemon = PdDaemon::new(c)?;
                        daemon.run();
                    },
                };
            }
        }
        Some(("stop", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                println!("stop: {}", dev.name());
            }
        }
        Some(("attach", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                println!("attach: {}", dev.name());
            }
        }
        _ => unreachable!(),
    }
    Ok(())
}