use std::{
    path::{PathBuf, Path},
    ffi::OsString,
    io::{ErrorKind, Error},
    process::Command, borrow::BorrowMut,
};

use anyhow::Context;
type Result<T> = anyhow::Result<T, anyhow::Error>;

/// For some reason build.rs cannot read non-source files located outside of
/// CARGO_MANIFEST_DIR when packaging. As a temporary work around, let's include
/// the contents of src/osdp_config.h.in here.
const OSDP_CONFIG_H: &str = r#"
#ifndef _OSDP_CONFIG_H_
#define _OSDP_CONFIG_H_

#define PROJECT_VERSION                "@PROJECT_VERSION@"
#define PROJECT_NAME                   "@PROJECT_NAME@"
#define GIT_BRANCH                     "@GIT_BRANCH@"
#define GIT_REV                        "@GIT_REV@"
#define GIT_TAG                        "@GIT_TAG@"
#define GIT_DIFF                       "@GIT_DIFF@"
#define REPO_ROOT                      "@REPO_ROOT@"

#define OSDP_PD_SC_RETRY_MS                     (600 * 1000)
#define OSDP_PD_POLL_TIMEOUT_MS                 (50)
#define OSDP_PD_SC_TIMEOUT_MS                   (800)
#define OSDP_RESP_TOUT_MS                       (200)
#define OSDP_ONLINE_RETRY_WAIT_MAX_MS           (300 * 1000)
#define OSDP_CMD_RETRY_WAIT_MS                  (300)
#define OSDP_PACKET_BUF_SIZE                    (256)
#define OSDP_RX_RB_SIZE                         (512)
#define OSDP_CP_CMD_POOL_SIZE                   (32)
#define OSDP_FILE_ERROR_RETRY_MAX               (10)
#define OSDP_PD_MAX                             (126)
#define OSDP_CMD_ID_OFFSET                      (5)

#endif /* _OSDP_CONFIG_H_ */
"#;

fn get_repo_root() -> std::io::Result<String> {
    let path = std::env::current_dir()?;
    let mut path_ancestors = path.as_path().ancestors();

    while let Some(p) = path_ancestors.next() {
        let has_git = std::fs::read_dir(p)?
            .into_iter()
            .any(|p| {
                let ent = p.unwrap();
                if ent.file_type().unwrap().is_dir() {
                    ent.file_name() == OsString::from(".git")
                } else {
                    false
                }
            });
        if has_git {
            return Ok(PathBuf::from(p).into_os_string().into_string().unwrap())
        }
    }

    let mut path_ancestors = path.as_path().ancestors();
    while let Some(p) = path_ancestors.next() {
        let has_cargo =
            std::fs::read_dir(p)?
                .into_iter()
                .any(|p| p.unwrap().file_name() == OsString::from("Cargo.lock"));
        if has_cargo {
            return Ok(PathBuf::from(p).into_os_string().into_string().unwrap())
        }
    }

    Err(Error::new(ErrorKind::NotFound, "Unable to determine repo root"))
}

fn path_join(root: &str, path: &str) -> String {
    Path::new(root).join(path).into_os_string().into_string().unwrap()
}

fn configure_file(mut contents: String, dest_path: &str,
                  transforms: Vec<(&str, &str)>) -> Result<()> {
    for (from, to) in transforms {
        if let Some(start) = contents.find(format!("@{from}@").as_str()) {
            let range = start..start + from.len() + 2;
            contents.replace_range(range, to);
        }
    }
    std::fs::write(dest_path, contents)?;
    Ok(())
}

fn exec_cmd(cmd: Vec<&str>) -> Result<String> {
    let mut c = Command::new(cmd[0]);
    let mut c = c.borrow_mut();
    for arg in &cmd[1..] {
        c = c.arg(*arg);
    }
    let stdout = String::from_utf8_lossy(&c.output()?.stdout).into_owned();
    Ok(stdout.trim().to_owned())
}

struct GitInfo {
    branch: String,
    tag: String,
    diff: String,
    rev: String,
    root: String,
}

impl GitInfo {
    pub fn new() -> Result<Self> {
        let diff = match exec_cmd(vec![ "git", "diff", "--quiet", "--exit-code" ]) {
            Ok(_) => "",
            Err(_) => "+",
        };
        Ok(GitInfo {
            branch: exec_cmd(vec!["git", "rev-parse", "--abbrev-ref", "HEAD"])?,
            tag: exec_cmd(vec![ "git", "describe", "--exact-match", "--tags" ]).unwrap_or("".to_owned()),
            diff: diff.to_owned(),
            rev: exec_cmd(vec!["git", "log", "--pretty=format:'%h'", "-n", "1"])?,
            root: exec_cmd(vec!["git", "rev-parse", "--show-toplevel"])?,
        })
    }
}

fn generate_osdp_build_headers(out_dir: &str) -> Result<()> {
    // Add an empty file as we don't
    let data = "#define OSDP_EXPORT";
    std::fs::write(
        path_join(&out_dir, "osdp_export.h"),
        data
    ).context("Failed to create osdp_export.h")?;

    let git = GitInfo::new()?;
    let dest = path_join(&out_dir, "osdp_config.h");
    configure_file(OSDP_CONFIG_H.to_owned(), &dest, vec![
        ("PROJECT_VERSION", env!("CARGO_PKG_VERSION")),
        ("PROJECT_NAME", format!("{}-rust", env!("CARGO_PKG_NAME")).as_str()),
        ("GIT_BRANCH", git.branch.as_str()),
        ("GIT_REV", git.rev.as_ref()),
        ("GIT_TAG", git.tag.as_ref()),
        ("GIT_DIFF", git.diff.as_ref()),
        ("REPO_ROOT", git.root.as_ref()),
    ])
}

fn main() -> Result<()> {
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let repo_root = get_repo_root()?;

    generate_osdp_build_headers(&out_dir)?;

    let mut build = cc::Build::new();
    let mut build = build
        .include(path_join(&repo_root, "src"))
        .include(path_join(&repo_root, "include"))
        .include(path_join(&repo_root, "utils/include"))
        .warnings(true)
        .warnings_into_errors(true)
        .include(&out_dir);

    let source_files = vec![
        "utils/src/list.c",
        "utils/src/queue.c",
        "utils/src/slab.c",
        "utils/src/utils.c",
        "utils/src/logger.c",
        "utils/src/disjoint_set.c",
        "src/osdp_common.c",
        "src/osdp_phy.c",
        "src/osdp_sc.c",
        "src/osdp_file.c",
        "src/osdp_pd.c",
        "src/osdp_cp.c",
        "src/crypto/tinyaes_src.c",
        "src/crypto/tinyaes.c"
    ];

    for file in source_files {
        let path = path_join(&repo_root, file);
        // Tell Cargo that if the given file changes, to rerun this build script.
        println!("cargo:rerun-if-changed={}", path);
        build = build.file(path);
    }

    if cfg!(feature = "skip_mark_byte") {
        build = build.define("CONFIG_OSDP_SKIP_MARK_BYTE", "1");
    }

    if cfg!(feature = "packet_trace") {
        build = build.define("CONFIG_OSDP_PACKET_TRACE", "1");
    }

    if cfg!(feature = "data_trace") {
        build = build.define("CONFIG_OSDP_DATA_TRACE", "1");
    }

    build.compile("libosdp.a");

    let bindings = bindgen::Builder::default()
        .header(path_join(&repo_root, "include/osdp.h"))
        .generate()
        .context("Unable to generate bindings")?;

    let out_path = PathBuf::from(out_dir);
    bindings.write_to_file(out_path.join("bindings.rs"))
        .context("Couldn't write bindings!")
}
