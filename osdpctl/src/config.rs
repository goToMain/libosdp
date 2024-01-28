use configparser::ini::Ini;
use libosdp::{
    channel::{UnixChannel, OsdpChannel},
    OsdpFlag, PdCapability, PdId, PdInfo,
};
use std::{
    fmt::Write,
    path::{Path, PathBuf},
    str::FromStr,
};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct AppConfig {
    pub device_config_dir: PathBuf,
}

impl AppConfig {
    pub fn from(cfg: &Path) -> Result<Self> {
        if !cfg.exists() {
            let mut dir = PathBuf::from(cfg);
            dir.pop();
            return Ok(Self {
                device_config_dir: dir,
            });
        }
        let mut config = Ini::new();
        config.load(cfg).map_err(|e| anyhow::anyhow!(e))?;
        let s = config.get("default", "DeviceDir").unwrap();
        let device_config_dir = if s.starts_with("./") {
            let mut path = PathBuf::from(cfg);
            path.pop();
            path.push(&s[2..]);
            path = std::fs::canonicalize(&s)?;
            path
        } else {
            let path = std::fs::canonicalize(&s)?;
            path
        };
        Ok(Self { device_config_dir })
    }
}

fn vec_to_array<T, const N: usize>(v: Vec<T>) -> [T; N] {
    v.try_into()
        .unwrap_or_else(|v: Vec<T>| panic!("Expected a Vec of length {} but it was {}", N, v.len()))
}

pub struct KeyStore {
    store: PathBuf,
}

impl KeyStore {
    pub fn from_path(path: &str) -> Self {
        let store = PathBuf::from_str(path).unwrap();
        Self { store }
    }

    pub fn decode_hex(&self, s: &str) -> anyhow::Result<Vec<u8>, std::num::ParseIntError> {
        (0..s.len())
            .step_by(2)
            .map(|i| u8::from_str_radix(&s[i..i + 2], 16))
            .collect()
    }

    fn str_to_key(&self, s: &str) -> Result<[u8; 16]> {
        let key = self.decode_hex(s)?;
        Ok(vec_to_array::<u8, 16>(key))
    }

    fn key_to_str(&self, key: &[u8; 16]) -> String {
        let mut s = String::with_capacity(key.len() * 2);
        for b in key {
            write!(&mut s, "{:02x}", b).unwrap();
        }
        s
    }

    pub fn load(&self) -> Result<[u8; 16]> {
        if !self.store.exists() {
            anyhow::bail!("Store does not exist");
        }
        let s = std::fs::read_to_string(self.store.as_path())?;

        self.str_to_key(&s)
    }

    pub fn store(&self, key: [u8; 16]) -> Result<()> {
        std::fs::write(&self.store, self.key_to_str(&key))?;
        Ok(())
    }
}

pub struct PdData {
    pub name: String,
    channel: String,
    address: i32,
    pub key_store: KeyStore,
    flags: OsdpFlag,
}

pub struct CpConfig {
    pub name: String,
    pd_data: Vec<PdData>,
    pub pid_file: PathBuf,
    pub log_dir: PathBuf,
    pub log_level: String,
}

impl CpConfig {
    pub fn from(config: &Ini) -> Result<Self> {
        let name = config.get("default", "Name").unwrap();
        let num_pd = config.getuint("default", "NumPd").unwrap().unwrap() as usize;
        let mut pd_data = Vec::new();
        for pd in 0..num_pd {
            let section = format!("PD-{pd}");
            pd_data.push(PdData {
                name: config.get(&section, "Name").unwrap(),
                channel: config.get(&section, "Channel").unwrap(),
                address: config.getuint(&section, "Address").unwrap().unwrap() as i32,
                key_store: KeyStore::from_path(&config.get("default", "KeyStore").unwrap()),
                flags: OsdpFlag::empty(),
            });
        }
        let pid_file = PathBuf::from_str(&config.get("Service", "PidFile").unwrap())?;
        let log_dir = PathBuf::from_str(&config.get("Service", "LogDir").unwrap())?;
        let log_level = config.get("Service", "LogLevel").unwrap();
        Ok(Self {
            name,
            pd_data,
            pid_file,
            log_level,
            log_dir,
        })
    }

    pub fn pd_info(&self) -> Result<Vec<PdInfo>> {
        self.pd_data
            .iter()
            .map(|d| {
                let stream = UnixChannel::new(&d.channel)?;
                Ok(PdInfo::for_cp(
                    &self.name,
                    d.address,
                    9600,
                    d.flags,
                    OsdpChannel::new::<UnixChannel>(Box::new(stream)),
                    d.key_store.load()?,
                ))
            })
            .collect()
    }
}

pub struct PdConfig {
    pub name: String,
    channel: String,
    address: i32,
    pub key_store: KeyStore,
    pd_id: PdId,
    pd_cap: Vec<PdCapability>,
    flags: OsdpFlag,
    pub pid_file: PathBuf,
    pub log_level: String,
    pub log_dir: PathBuf,
}

impl PdConfig {
    pub fn from(config: &Ini) -> Result<Self> {
        // TODO: Fix this
        let pd_id = PdId::from_number(1);
        // let pd_id = PdId {
        //     version: config.getuint("PdId", "Version").unwrap().unwrap() as i32,
        //     model: config.getuint("PdId", "Model").unwrap().unwrap() as i32,
        //     vendor_code: config.getuint("PdId", "VendorCode").unwrap().unwrap() as u32,
        //     serial_number: config.getuint("PdId", "SerialNumber").unwrap().unwrap() as u32,
        //     firmware_version: config.getuint("PdId", "FirmwareVersion").unwrap().unwrap() as u32,
        // };
        let mut flags = OsdpFlag::empty();
        if let Some(val) = config.get("default", "Flags") {
            let fl: Vec<&str> = val.split('|').collect();
            for f in fl {
                flags.set(OsdpFlag::from_str(f)?, true);
            }
        }
        let name = config.get("default", "Name").unwrap();
        let channel = config.get("default", "Channel").unwrap();
        let address = config.getuint("default", "Address").unwrap().unwrap() as i32;
        let key_store = KeyStore::from_path(&config.get("default", "KeyStore").unwrap());
        let map = config.get_map().unwrap();
        let cap_map = map.get("PdCapability").unwrap();
        let mut pd_cap = Vec::new();
        for (key, val) in cap_map {
            pd_cap.push(PdCapability::from_str(
                format!("{}:{}", key, val.as_deref().unwrap()).as_str(),
            )?);
        }
        let pid_file = PathBuf::from_str(&config.get("Service", "PidFile").unwrap())?;
        let log_dir = PathBuf::from_str(&config.get("Service", "LogDir").unwrap())?;
        let log_level = config.get("Service", "LogLevel").unwrap();
        Ok(Self {
            name,
            channel,
            address,
            key_store,
            pd_id,
            pd_cap,
            flags,
            pid_file,
            log_level,
            log_dir,
        })
    }

    pub fn pd_info(&self) -> Result<PdInfo> {
        let stream = UnixChannel::new(&self.channel)?;
        Ok(PdInfo::for_pd(
            &self.name,
            self.address,
            9600,
            self.flags,
            self.pd_id.clone(),
            self.pd_cap.clone(),
            OsdpChannel::new::<UnixChannel>(Box::new(stream)),
            self.key_store.load()?,
        ))
    }
}

pub enum DeviceConfig {
    CpConfig(CpConfig),
    PdConfig(PdConfig),
}

impl DeviceConfig {
    pub fn from_path(cfg: &Path) -> Result<Self> {
        let mut config = Ini::new_cs();
        config.load(cfg).unwrap();
        let config = match config.get("default", "NumPd") {
            Some(_) => DeviceConfig::CpConfig(CpConfig::from(&config)?),
            None => DeviceConfig::PdConfig(PdConfig::from(&config)?),
        };
        Ok(config)
    }

    pub fn name(&self) -> &str {
        match self {
            DeviceConfig::CpConfig(c) => &c.name,
            DeviceConfig::PdConfig(c) => &c.name,
        }
    }
}
