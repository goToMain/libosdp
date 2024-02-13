//
// Copyright (c) 2023-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

use anyhow::Context;
use daemonize::Daemonize;
use std::path::Path;

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub fn daemonize(runtime_dir: &Path, name: &str) -> Result<()> {
    let stdout = std::fs::File::create(runtime_dir.join(format!("dev-{}.out.log", name).as_str()))
        .context("Failed to create stdout for daemon")?;
    let stderr = std::fs::File::create(runtime_dir.join(format!("dev-{}.err.log", name).as_str()))
        .context("Failed to create stderr for daemon")?;
    let daemon = Daemonize::new()
        .pid_file(runtime_dir.join(format!("dev-{}.pid", name)))
        .chown_pid_file(true)
        .working_directory(runtime_dir)
        .stdout(stdout)
        .stderr(stderr);
    daemon.start().context("Failed to start daemon process")?;
    Ok(())
}
