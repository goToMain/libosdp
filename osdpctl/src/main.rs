use std::{
    path::{PathBuf, Path},
    collections::HashMap,
    str::FromStr
};

use clap::{arg, Command};
use configparser::ini::Ini;
use libosdp::common::{PdId, PdCapability, OsdpFlag};

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

struct AppConfig {
    device_config_dir: PathBuf
}

impl AppConfig {
    pub fn from(cfg: &Path) -> Result<Self> {
        let mut config = Ini::new();
        let _ = config.load(cfg).unwrap();
        let s = config.get("default", "device_config_dir").unwrap();
        let device_config_dir = if s.starts_with("./") {
            let mut path = PathBuf::from(cfg);
            path.pop();
            path.push(&s[2..]);
            path
        } else {
            PathBuf::from(s)
        };
        Ok(Self {
            device_config_dir,
        })
    }
}

struct PdData {
    name: String,
    channel: String,
    address: u8,
    key_store: PathBuf,
    flags: Option<Vec<OsdpFlag>>
}

struct Device {
    is_pd_mode: bool,
    name: String,
    num_pd: usize,
    log_level: String,

    pid_file: PathBuf,

    pd_id: Option<PdId>, // pd
    pd_cap: Option<Vec<PdCapability>>, //pd
    pd_data: Vec<PdData>,
}

impl Device {
    pub fn from(cfg: &Path) -> Result<Self> {
        let mut config = Ini::new_cs();
        let map = config.load(cfg).unwrap();

        let name = config.get("default", "Name").unwrap();
        let log_level = config.get("Service", "LogLevel").unwrap();
        let pid_file = PathBuf::from_str(&config.get("Service", "PidFile").unwrap())?;

        let is_pd_mode = config.get("default", "Mode").unwrap() == "PD";

        let mut pd_id = None;
        let mut num_pd = 1;
        let mut pd_data = Vec::new();
        let mut pd_cap: Option<Vec<PdCapability>> = None;
        if is_pd_mode {
            pd_id = Some(PdId {
                version: config.getuint("PdId", "Version").unwrap().unwrap() as i32,
                model: config.getuint("PdId", "Model").unwrap().unwrap() as i32,
                vendor_code: config.getuint("PdId", "VendorCode").unwrap().unwrap() as u32,
                serial_number: config.getuint("PdId", "SerialNumber").unwrap().unwrap() as u32,
                firmware_version: config.getuint("PdId", "FirmwareVersion").unwrap().unwrap() as u32
            });
            let mut flags = Vec::new();
            if let Some(val) = config.get("default", "Flags") {
                let fl: Vec<&str> = val.split('|').collect();
                for f in fl {
                    flags.push(OsdpFlag::from_str(f)?);
                }
            }
            pd_data.push(PdData {
                name: config.get("default", "Name").unwrap(),
                channel: config.get("default", "Channel").unwrap(),
                address: config.getuint("default", "Address").unwrap().unwrap() as u8,
                key_store: PathBuf::from_str(&config.get("default", "KeyStore").unwrap())?,
                flags: Some(flags),
            });
            let cap_map = map.get("PdCapability").unwrap();
            let mut cap = Vec::new();
            for (key, val) in cap_map {
                cap.push(PdCapability::from_str(format!("{}:{}", key, val.as_deref().unwrap()).as_str())?);
            }
            pd_cap = Some(cap);
        } else {
            num_pd = config.getuint("default", "NumPd").unwrap().unwrap() as usize;
            for pd in 0..num_pd {
                let section = format!("PD-{pd}");
                pd_data.push(PdData {
                    name: config.get(&section, "Name").unwrap(),
                    channel: config.get(&section, "Channel").unwrap(),
                    address: config.getuint(&section, "Address").unwrap().unwrap() as u8,
                    key_store: PathBuf::from_str(&config.get(&section, "KeyStore").unwrap())?,
                    flags: None,
                });
            }
        }

        let device = Self {
            name,
            is_pd_mode,
            num_pd,
            log_level,
            pid_file,
            pd_id,
            pd_data,
            pd_cap,
        };
        Ok(device)
    }

    pub fn status(&self) -> String {
        String::from_str("online").unwrap()
    }
}

fn main() -> Result<()> {
    let cfg_dir = osdpctl_config_dir()?;
    let config = AppConfig::from(&cfg_dir.join("osdpctl.cfg"))?;

    let paths = std::fs::read_dir(&config.device_config_dir).unwrap();
    let mut device_map: HashMap<String, Device> = HashMap::new();
    for path in paths {
        let path = path.unwrap().path();
        if let Some(ext) = path.extension() {
            if ext == "cfg" {
                let dev = Device::from(&path)?;
                device_map.insert(dev.name.clone(), dev);
            }
        }
    }

    match cli().get_matches().subcommand() {
        Some(("list", _)) => {
            for (name, device) in device_map {
                println!("{}: {}", name, device.status());
            }
        }
        Some(("start", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                println!("start: {}", dev.name);
            }
        }
        Some(("stop", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                println!("stop: {}", dev.name);
            }
        }
        Some(("attach", sub_matches)) => {
            let name = sub_matches.get_one::<String>("DEV")
                .expect("device name is required");
            if let Some(dev) = device_map.get(name) {
                println!("attach: {}", dev.name);
            }
        }
        _ => unreachable!(),
    }
    Ok(())
}