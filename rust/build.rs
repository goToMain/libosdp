use std::path::PathBuf;

use anyhow::Context;
type Result<T> = anyhow::Result<T, anyhow::Error>;

fn main() -> Result<()> {
    // Tell Cargo that if the given file changes, to rerun this build script.
    // println!("cargo:rerun-if-changed=src/hello.c");
    // Use the `cc` crate to build a C file and statically link it.
    let repo_root = std::fs::canonicalize("../")?.into_os_string().into_string().unwrap();
    let mut build = cc::Build::new();
    let builder = build
        .include("../src/")
        .include("../include/")
        .include("../utils/include/")
        .define("REPO_ROOT", format!("\"{}\"", repo_root.as_str()).as_str())
        .define("CONFIG_NO_GENERATED_HEADERS", None)
        .define("PROJECT_VERSION", format!("\"{}\"", "2.2.0").as_str())
        .define("PROJECT_NAME", format!("\"{}\"", "libosdp").as_str())
        .define("GIT_BRANCH", format!("\"{}\"", "master").as_str())
        .define("GIT_REV", format!("\"{}\"", "6dc2621").as_str())
        .define("GIT_TAG", "\"\"")
        .define("GIT_DIFF", format!("\"{}\"", "+").as_str())
        .define("OSDP_PD_SC_RETRY_MS", "600000")
        .define("OSDP_PD_POLL_TIMEOUT_MS", "50")
        .define("OSDP_PD_SC_TIMEOUT_MS", "800")
        .define("OSDP_RESP_TOUT_MS", "200")
        .define("OSDP_ONLINE_RETRY_WAIT_MAX_MS", "300000")
        .define("OSDP_CMD_RETRY_WAIT_MS", "300")
        .define("OSDP_PACKET_BUF_SIZE", "256")
        .define("OSDP_RX_RB_SIZE", "512")
        .define("OSDP_CP_CMD_POOL_SIZE", "32")
        .define("OSDP_FILE_ERROR_RETRY_MAX", "10")
        .define("OSDP_PD_MAX", "126")
        .define("OSDP_CMD_ID_OFFSET", "5")
        .define("OSDP_EXPORT", "")
        .define("OSDP_NO_EXPORT", "")
        .define("OSDP_DEPRECATED_EXPORT", "")
        .warnings(true)
        .warnings_into_errors(true)
        .file("../utils/src/list.c")
        .file("../utils/src/queue.c")
        .file("../utils/src/slab.c")
        .file("../utils/src/utils.c")
        .file("../utils/src/disjoint_set.c")
        .file("../utils/src/logger.c")
        .file("../src/osdp_common.c")
        .file("../src/osdp_phy.c")
        .file("../src/osdp_sc.c")
        .file("../src/osdp_file.c")
        .file("../src/osdp_pd.c")
        .file("../src/osdp_cp.c")
        .file("../src/crypto/tinyaes_src.c")
        .file("../src/crypto/tinyaes.c");

    builder.compile("libosdp.a");

    let bindings = bindgen::Builder::default()
        .header("../include/osdp.h")
        .generate()
        .context("Unable to generate bindings")?;

    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings.write_to_file(out_path.join("bindings.rs"))
        .context("Couldn't write bindings!")
}
