//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

mod config;
mod cp;
mod daemonize;
mod pd;

use anyhow::{bail, Context};
use clap::{arg, Command};
use config::DeviceConfig;
use log::LevelFilter;
use log4rs::{
    append::console::ConsoleAppender,
    config::{Appender, Root},
    Config,
};
use nix::{
    sys::signal::{self, Signal},
    unistd::Pid,
};
use std::{path::PathBuf, str::FromStr};
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
                .arg(arg!(-d --daemonize "Fork and run in the background"))
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

fn main() -> Result<()> {
    let lh = log4rs::init_config(get_logger_config(LevelFilter::Info)?)?;
    let cfg_dir = osdpctl_config_dir()?;
    let rt_dir = device_runtime_dir()?;
    let matches = cli().get_matches();
    match matches.subcommand() {
        Some(("edit", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("Device name is required")?;
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let editor = std::env::var("EDITOR")
                .context("Environment variable EDITOR is not set")?;
            std::process::Command::new(editor)
                .arg(&config_path)
                .status()
                .context("External editor returned error code")?;
        }
        Some(("create", sub_matches)) => {
            let config = sub_matches
                .get_one::<String>("CONFIG")
                .context("Device config file required")?;
            let config = PathBuf::from_str(&config)?;
            let dev = DeviceConfig::new(&config, &rt_dir)?;
            let dest_path = cfg_dir.join(format!("{}.cfg", dev.name()));
            if dest_path.exists() {
                bail!("A device config with the name '{}' already exists!", dev.name());
            }
            std::fs::copy(&config, &dest_path).unwrap();
            println!("Created new device '{}'.", dev.name())
        }
        Some(("destroy", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("Device name is required")?;
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            if !config_path.exists() {
                bail!(format!("Device '{}' not found. See `osdpctl list`.", name));
            }
            let sock = &rt_dir.join(format!("{name}/{name}.sock"));
            if sock.exists() {
                bail!("Device '{name}' is still running; stop it first.");
            }
            std::fs::remove_file(config_path).unwrap();
            println!("Destroyed device '{name}'.")
        }
        Some(("list", _)) => {
            let paths = std::fs::read_dir(&cfg_dir).unwrap();
            println!("  Nr  Device Name     Status   ");
            println!("-------------------------------");
            for (i, path) in paths.enumerate() {
                let path = path.unwrap().path();
                if let Some(ext) = path.extension() {
                    if ext == "cfg" {
                        let dev = DeviceConfig::new(&path, &rt_dir)?;
                        println!("  {:02}  {:<13}   {:^8}  ", i, dev.name(), "Offline");
                    }
                }
            }
        }
        Some(("start", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("Device name is required")?;
            let daemonize = sub_matches.get_flag("daemonize");
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            match dev {
                DeviceConfig::CpConfig(dev) => {
                    lh.set_config(get_logger_config(dev.log_level)?);
                    cp::main(dev, daemonize)?;
                }
                DeviceConfig::PdConfig(dev) => {
                    lh.set_config(get_logger_config(dev.log_level)?);
                    pd::main(dev, daemonize)?;
                }
            };
        }
        Some(("stop", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("Device name is required")?;
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            let pid = dev.get_pid()?;
            signal::kill(Pid::from_raw(pid), Signal::SIGHUP)
                .context("Failed to stop to requested device")?;
            println!("Device `{}` stopped", dev.name());
        }
        Some(("attach", sub_matches)) => {
            let name = sub_matches
                .get_one::<String>("DEV")
                .context("device name is required")?;
            let config_path = cfg_dir.join(format!("{name}.cfg"));
            let dev = DeviceConfig::new(&config_path, &rt_dir)?;
            println!("attach: {}", dev.name());
            todo!();
        }
        _ => bail!("Unknown command"),
    }
    Ok(())
}
