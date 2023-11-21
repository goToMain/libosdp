#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import re
from setuptools import setup, Extension
import shutil
import subprocess

current_dir = os.environ.get("PWD")
build_dir = os.path.abspath(os.path.join(current_dir, "build"))
root_dir = os.path.abspath(os.path.join(current_dir, ".."))

def read_version():
    with open(os.path.join(root_dir, "CMakeLists.txt")) as f:
        for line in f.readlines():
            m = re.match(r"^project\((.+) VERSION (\d+\.\d+\.\d+)\)$", line)
            if (m):
                return m.groups()
    raise RuntimeError("Failed to parse package name and version")

project_name, project_version = read_version()

def map_prefix(src_list, path, check_files=False):
    paths = [ os.path.join(path, src) for src in src_list ]
    for path in paths:
        if check_files:
            if not os.path.exists(path):
                raise RuntimeError(f"Path '{path}' does not exist")
    return paths

def exec_cmd(cmd):
    r = subprocess.run(cmd, capture_output = True, text = True)
    return r.stdout.strip()

def get_git_info():
    d = {}
    d["branch"] = exec_cmd(["git", "rev-parse", "--abbrev-ref", "HEAD"])
    d["tag"] = exec_cmd([ "git", "describe", "--exact-match", "--tags" ])
    d["diff"] = exec_cmd([ "git", "diff", "--quiet", "--exit-code" ])
    d["rev"] = exec_cmd(["git", "log", "--pretty=format:%h", "-n", "1"])
    d["root"] = exec_cmd(["git", "rev-parse", "--show-toplevel"])
    return d

def configure_file(file, replacements):
    with open(file) as f:
        contents = f.read()
    pat = re.compile(r"@(\w+)@")
    for match in pat.findall(contents):
        if match in replacements:
            contents = contents.replace(f"@{match}@", replacements[match])
    with open(file, 'w') as f:
        f.write(contents)

def generate_osdp_build_headers():
    os.makedirs(build_dir, exist_ok=True)
    with open(os.path.join(build_dir, "osdp_export.h"), 'w') as f:
        f.write("#define OSDP_EXPORT\n")

    git = get_git_info()
    dest = os.path.join(build_dir, "osdp_config.h")
    shutil.copy(os.path.join(root_dir, "src/osdp_config.h.in"), dest)
    configure_file(dest, {
        "PROJECT_VERSION": project_version,
        "PROJECT_NAME": project_name,
        "GIT_BRANCH": git["branch"],
        "GIT_REV": git["rev"],
        "GIT_TAG": git["tag"],
        "GIT_DIFF": git["diff"],
        "REPO_ROOT": git["root"],
    })

generate_osdp_build_headers()

utils_sources = map_prefix([
    "utils/src/list.c",
    "utils/src/queue.c",
    "utils/src/slab.c",
    "utils/src/utils.c",
    "utils/src/logger.c",
    "utils/src/disjoint_set.c",
    "utils/src/serial.c",
    "utils/src/channel.c",
    "utils/src/hashmap.c",
    "utils/src/strutils.c",
    "utils/src/memory.c",
    "utils/src/fdutils.c",
    "utils/src/event.c",
    "utils/src/workqueue.c",
    "utils/src/sockutils.c",
    "utils/src/bus_server.c",
], root_dir)

lib_sources = map_prefix([
    "src/osdp_common.c",
    "src/osdp_phy.c",
    "src/osdp_sc.c",
    "src/osdp_file.c",
    "src/osdp_pd.c",
    "src/osdp_cp.c",
    "src/crypto/tinyaes_src.c",
    "src/crypto/tinyaes.c",
], root_dir)

osdp_sys_sources = map_prefix([
    "osdp_sys/module.c",
    "osdp_sys/base.c",
    "osdp_sys/cp.c",
    "osdp_sys/pd.c",
    "osdp_sys/data.c",
    "osdp_sys/utils.c",
], current_dir)

source_files = utils_sources + lib_sources + osdp_sys_sources

include_dirs = map_prefix([
    "utils/include",
    "include",
    "src",
    "python/build",
], root_dir)

other_files = map_prefix([
    "CMakeLists.txt",
    "src/osdp_config.h.in"
], root_dir)

include_files = map_prefix([
    "utils/include/utils/list.h",
    "utils/include/utils/queue.h",
    "utils/include/utils/slab.h",
    "utils/include/utils/utils.h",
    "utils/include/utils/logger.h",
    "utils/include/utils/disjoint_set.h",
    "utils/include/utils/serial.h",
    "utils/include/utils/channel.h",
    "utils/include/utils/hashmap.h",
    "utils/include/utils/strutils.h",
    "utils/include/utils/memory.h",
    "utils/include/utils/fdutils.h",
    "utils/include/utils/event.h",
    "utils/include/utils/workqueue.h",
    "utils/include/utils/sockutils.h",
    "utils/include/utils/bus_server.h",
    "include/osdp.h",
    "src/osdp_common.h",
    "src/osdp_file.h",
    "python/build/osdp_config.h",
    "python/build/osdp_export.h",
], root_dir, check_files=True)

libosdp_includes = [ "-I" + path for path in include_dirs ]

definitions = [
    # "CONFIG_OSDP_PACKET_TRACE",
    # "CONFIG_OSDP_DATA_TRACE",
    # "CONFIG_OSDP_SKIP_MARK_BYTE",
]

compile_args = (
    [ "-I" + path for path in include_dirs ] +
    [ "-D" + define for define in definitions ]
)

link_args = []

if os.path.exists("README.md"):
    with open("README.md", "r") as f:
        long_description = f.read()
else:
    long_description = ""

setup(
    name         = project_name,
    version      = project_version,
    author       = "Siddharth Chandrasekaran",
    author_email = "sidcha.dev@gmail.com",
    description  = "Library implementation of IEC 60839-11-5 OSDP (Open Supervised Device Protocol)",
    url          = "https://github.com/goToMain/libosdp",
    ext_modules  = [
        Extension(
            name               = "osdp_sys",
            sources            = source_files,
            extra_compile_args = compile_args,
            extra_link_args    = link_args,
            define_macros      = [],
            language           = "C",
        )
    ],
    packages     = [ "osdp" ],
    classifiers  = [
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    long_description              = long_description,
    long_description_content_type = "text/markdown",
    python_requires               = ">=3.8",
    package_data = { project_name : other_files }
)
