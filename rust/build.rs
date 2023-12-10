use std::{
    path::{PathBuf, Path},
    ffi::OsString,
    io::{ErrorKind, Error},
    process::Command, borrow::BorrowMut, str::FromStr,
};

use anyhow::Context;
type Result<T> = anyhow::Result<T, anyhow::Error>;

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

fn configure_file(path: &str, transforms: Vec<(&str, &str)>) -> Result<()>{
    let mut contents = std::fs::read_to_string(path)?;

    for (from, to) in transforms {
        if let Some(start) = contents.find(format!("@{from}@").as_str()) {
            let range = start..start + from.len() + 2;
            contents.replace_range(range, to);
        }
    }
    std::fs::write(path, contents)?;
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
    let src = "vendor/src/osdp_config.h.in";
    let dest = path_join(&out_dir, "osdp_config.h");
    std::fs::copy(&src, &dest)
        .context(format!("Failed: copy {src} -> {dest}"))?;
    configure_file(&dest, vec![
            ("PROJECT_VERSION", env!("CARGO_PKG_VERSION")),
        ("PROJECT_NAME", format!("{}-rust", env!("CARGO_PKG_NAME")).as_str()),
            ("GIT_BRANCH", git.branch.as_str()),
            ("GIT_REV", git.rev.as_ref()),
            ("GIT_TAG", git.tag.as_ref()),
            ("GIT_DIFF", git.diff.as_ref()),
            ("REPO_ROOT", git.root.as_ref()),
    ])
}

fn dirname_mkdir(path: &Path) {
    let mut dir = PathBuf::from(path);
    dir.pop();
    std::fs::create_dir_all(&dir).unwrap();
}

fn vendor_sources(root: &str, vendor: &str) -> Result<()> {
    let sources = vec![
        "utils/include/utils/assert.h",
        "utils/src/list.c",
        "utils/include/utils/list.h",
        "utils/src/queue.c",
        "utils/include/utils/queue.h",
        "utils/src/slab.c",
        "utils/include/utils/slab.h",
        "utils/src/utils.c",
        "utils/include/utils/utils.h",
        "utils/src/logger.c",
        "utils/include/utils/logger.h",
        "utils/src/disjoint_set.c",
        "utils/include/utils/disjoint_set.h",
        "include/osdp.h",
        "src/osdp_common.c",
        "src/osdp_common.h",
        "src/osdp_config.h.in",
        "src/osdp_phy.c",
        "src/osdp_sc.c",
        "src/osdp_file.c",
        "src/osdp_file.h",
        "src/osdp_pd.c",
        "src/osdp_cp.c",
        "src/crypto/tinyaes_src.c",
        "src/crypto/tinyaes_src.h",
        "src/crypto/tinyaes.c",
    ];
    let root = PathBuf::from_str(root)?;
    for file in sources {
        let src = root.clone().join(Path::new(&file));
        let mut dest = PathBuf::from(vendor);
        dest.push(file);
        dirname_mkdir(&dest);
        // Tell Cargo that if the given file changes, to rerun this build script.
        println!("cargo:rerun-if-changed={}", src.display());
        std::fs::copy(&src, &dest)?;
    }
    Ok(())
}

fn main() -> Result<()> {
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let repo_root = get_repo_root()?;
    let vendor = concat!(env!("CARGO_MANIFEST_DIR"), "/vendor").to_owned();

    let _ = vendor_sources(&repo_root, &vendor);

    generate_osdp_build_headers(&out_dir)?;

    let mut build = cc::Build::new();
    let mut build = build
        .include("vendor/src")
        .include("vendor/include")
        .include("vendor/utils/include")
        .warnings(true)
        .warnings_into_errors(true)
        .include(&out_dir);

    let source_files = vec![
        "vendor/utils/src/list.c",
        "vendor/utils/src/queue.c",
        "vendor/utils/src/slab.c",
        "vendor/utils/src/utils.c",
        "vendor/utils/src/logger.c",
        "vendor/utils/src/disjoint_set.c",
        "vendor/src/osdp_common.c",
        "vendor/src/osdp_phy.c",
        "vendor/src/osdp_sc.c",
        "vendor/src/osdp_file.c",
        "vendor/src/osdp_pd.c",
        "vendor/src/osdp_cp.c",
        "vendor/src/crypto/tinyaes_src.c",
        "vendor/src/crypto/tinyaes.c"
    ];

    for file in source_files {
        build = build.file(file);
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
        .use_core()
        .header("vendor/include/osdp.h")
        .generate()
        .context("Unable to generate bindings")?;

    let out_path = PathBuf::from(out_dir);
    bindings.write_to_file(out_path.join("bindings.rs"))
        .context("Couldn't write bindings!")
}
