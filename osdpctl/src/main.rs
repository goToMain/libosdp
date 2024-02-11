mod config;
mod cp;
mod pd;

use std::{path::{Path, PathBuf}, str::FromStr, thread, time::Duration};

use anyhow::{bail, Context};
use clap::{arg, Command};
use config::DeviceConfig;
use log::LevelFilter;
use log4rs::{
    append::console::ConsoleAppender,
    config::{Appender, Root},
    Config,
};

type Result<T> = anyhow::Result<T, anyhow::Error>;

const HELP_TEMPLATE: &str = "{before-help}
{name} - {about}

{usage-heading} {usage}

{all-args}

Please report bugs to {author}
{after-help}";

fn cli() -> Command {
    Command::new(std::env!("CARGO_PKG_NAME"))
        .about(std::env!("CARGO_PKG_DESCRIPTION"))
        .version(std::env!("CARGO_PKG_VERSION"))
        .author(std::env!("CARGO_PKG_AUTHORS"))
        .help_template(HELP_TEMPLATE)
        .subcommand_required(true)
        .arg_required_else_help(true)
        .subcommand(
            Command::new("list")
                .about("List configured OSDP devices")
        )
        .subcommand(
            Command::new("create")
                .about("Create a device specified by config")
                .arg(arg!(<CONFIG> "device config file"))
                .arg_required_else_help(true),
        )
        .subcommand(
            Command::new("destroy")
                .about("Destroy device")
                .arg(arg!(<DEV> "device to destroy"))
                .arg_required_else_help(true),
        )
        .subcommand(
            Command::new("edit")
                .about("Edit device config")
                .arg(arg!(<DEV> "device to edit"))
                .arg_required_else_help(true),
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
        .arg(arg!(-d --daemonize "Fork and run in the background as a daemon"))
}

fn osdpctl_config_dir() -> Result<PathBuf> {
    let mut cfg_dir = dirs::config_dir()
        .expect("Failed to read system config directory");
    cfg_dir.push("osdp");
    _ = std::fs::create_dir_all(&cfg_dir);
    Ok(cfg_dir)
}

fn device_runtime_dir() -> Result<PathBuf> {
    let runtime_dir = dirs::runtime_dir()
        .unwrap_or(PathBuf::from_str("/tmp")?);
    std::fs::create_dir_all(&runtime_dir)?;
    Ok(runtime_dir)
}

fn get_logger_config(log_level: LevelFilter) -> Result<Config> {
    let stdout = ConsoleAppender::builder().build();
    let config = Config::builder()
        .appender(Appender::builder().build("stdout", Box::new(stdout)))
        .build(Root::builder().appender("stdout").build(log_level))?;
    Ok(config)
}

fn daemonize(runtime_dir: &Path, name: &str) {
    let stdout = std::fs::File::create(
        runtime_dir
            .join(format!("dev-{}.out.log", name).as_str()),
    ).expect("stdout for daemon");
    let stderr = std::fs::File::create(
        runtime_dir
            .join(format!("dev-{}.err.log", name).as_str()),
    ).expect("stderr for daemon");
    let daemon = daemonize::Daemonize::new()
        .pid_file(runtime_dir.join(format!("dev-{}.pid", name)))
        .chown_pid_file(true)
        .working_directory(runtime_dir)
        .stdout(stdout)
        .stderr(stderr);
    match daemon.start() {
        Ok(_) => {},
        Err(e) => eprintln!("Error, {}", e),
    }
}

fn main() -> Result<()> {
    let lh = log4rs::init_config(get_logger_config(LevelFilter::Info)?)?;
    let cfg_dir = osdpctl_config_dir()?;
    let rt_dir = device_runtime_dir()?;
    let matches = cli().get_matches();
    let run_as_daemon = matches.get_flag("daemonize");
    match matches.subcommand() {
        Some(("edit", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")
                .unwrap();
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            std::process::Command::new(std::env!("EDITOR"))
                .arg(&config_path)
                .status()
                .unwrap();
        },
        Some(("create", sub_matches)) => {
            let config = sub_matches
                .get_one::<String>("CONFIG")
                .context("device config file required")
                .unwrap();
            let config = PathBuf::from_str(&config)?;
            if !config.exists() {
                bail!("Config file does not exit");
            }
            let dev = DeviceConfig::new(&config, &rt_dir)?;
            let dest_path = cfg_dir.join(format!("{}.cfg", dev.name()));
            if dest_path.exists() {
                bail!("A device config with the name '{}' already exists!", dev.name());
            }
            std::fs::copy(&config, &dest_path).unwrap();
            println!("Created new device '{}'.", dev.name())
        },
        Some(("destroy", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")
                .unwrap();
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            if !config_path.exists() {
                bail!("No such device");
            }
            let sock = &rt_dir.join(format!("{name}/{name}.sock"));
            if sock.exists() {
                bail!("Device '{name}' is still running; stop it first.");
            }
            std::fs::remove_file(config_path).unwrap();
            println!("Destroyed device '{name}'.")
        },
        Some(("list", _)) => {
            let paths = std::fs::read_dir(&cfg_dir).unwrap();
            println!("+---------------+----------+");
            println!("|  Device Name  |  Status  |");
            println!("+---------------+----------+");
            for path in paths {
                let path = path.unwrap().path();
                if let Some(ext) = path.extension() {
                    if ext == "cfg" {
                        let dev = DeviceConfig::new(&path, &rt_dir)?;
                        println!("| {:<13} | {:^8} |", dev.name(), "Offline");
                    }
                }
            }
            println!("+---------------+----------+");
        }
        Some(("start", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")
                .unwrap();
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            if run_as_daemon {
                daemonize(&rt_dir, dev.name());
            }
            match dev {
                DeviceConfig::CpConfig(c) => {
                    lh.set_config(get_logger_config(c.log_level)?);
                    let mut cp = cp::create_cp(&c)
                        .context("Failed to create PD")?;
                    loop {
                        cp.refresh();
                        thread::sleep(Duration::from_millis(50));
                    }
                }
                DeviceConfig::PdConfig(c) => {
                    lh.set_config(get_logger_config(c.log_level)?);
                    let mut pd = pd::create_pd(&c)
                        .context("Failed to create PD")?;
                    loop {
                        pd.refresh();
                        thread::sleep(Duration::from_millis(50));
                    }
                }
            };
        }
        Some(("stop", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")
                .unwrap();
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            println!("stop: {}", dev.name());
            todo!();
        }
        Some(("attach", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")
                .unwrap();
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            println!("attach: {}", dev.name());
            todo!();
        }
        _ => bail!("Unknown command"),
    }
    Ok(())
}
