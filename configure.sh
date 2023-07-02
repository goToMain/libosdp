#!/bin/bash
#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

usage() {
	cat >&2<<----
	LibOSDP build configure script

	Configures a bare minimum build environemt for LibOSDP. This is not a
	replacement for cmake and intended only for those users who don't care
	about all the bells and whistles and need only the library.

	OPTIONS:
	  --packet-trace               Enable raw packet trace for diagnostics
	  --data-trace                 Enable command/reply data buffer tracing
	  --skip-mark                  Don't send the leading mark byte (0xFF)
	  --crypto LIB                 Use methods from LIB (openssl/mbedtls/*tinyaes)
	  --crypto-include-dir DIR     Include directory for crypto LIB if not in system path
	  --crypto-ld-flags            Args to pass to linker for the crypto LIB
	  --no-colours                 Don't colourize log ouputs
	  --static-pd                  Setup PD single statically
	  --lib-only                   Only build the library
	  --cross-compile PREFIX       Use to pass a compiler prefix
	  --build-dir                  Build output directory (default: ./build)
	  -d, --debug                  Enable debug builds
	  -f, --force                  Use this flags to override some checks
	  -h, --help                   Print this help
	---
}

BUILD_DIR="build"
SCRIPT_DIR="$(dirname $(readlink -f "$0"))"

while [ $# -gt 0 ]; do
	case $1 in
	--packet-trace)        PACKET_TRACE=1;;
	--data-trace)          DATA_TRACE=1;;
	--skip-mark)           SKIP_MARK_BYTE=1;;
	--cross-compile)       CROSS_COMPILE=$2; shift;;
	--crypto)              CRYPTO=$2; shift;;
	--crypto-include-dir)  CRYPTO_INCLUDE_DIR=$2; shift;;
	--crypto-ld-flags)     CRYPTO_LD_FLAGS=$2; shift;;
	--no-colours)          NO_COLOURS=1;;
	--static-pd)           STATIC_PD=1;;
	--lib-only)            LIB_ONLY=1;;
	--build-dir)           BUILD_DIR=$2; shift;;
	-d|--debug)            DEBUG=1;;
	-f|--force)            FORCE=1;;
	-h|--help)             usage; exit 0;;
	*) echo -e "Unknown option $1\n"; usage; exit 1;;
	esac
	shift
done

mkdir -p ${BUILD_DIR}

if [ -f config.make ] && [ -z "$FORCE" ]; then
	echo "LibOSDP already configured! Use --force to re-configure"
	exit 1
fi

if [[ ("$OSTYPE" == "cygwin") || ("$OSTYPE" == "win32") ]]; then
	echo "Warning: unsuported platform. Expect issues!"
fi

## Toolchains
CC=${CC:-${CROSS_COMPILE}gcc}
CXX=${CXX:-${CROSS_COMPILE}g++}
AR=${AR:-${CROSS_COMPILE}ar}

## check if submodules are initialized
if [[ -z "$(ls -A utils)" ]]; then
	"Initializing git-submodule utils"
	git submodule update --init
fi

## Build options
if [[ ! -z "${NO_COLOURS}" ]]; then
	CCFLAGS+=" -DCONFIG_DISABLE_PRETTY_LOGGING"
fi

if [[ ! -z "${PACKET_TRACE}" ]]; then
	CCFLAGS+=" -DCONFIG_OSDP_PACKET_TRACE"
fi

if [[ ! -z "${DATA_TRACE}" ]]; then
	CCFLAGS+=" -DCONFIG_OSDP_DATA_TRACE"
fi

if [[ ! -z "${SKIP_MARK_BYTE}" ]]; then
	CCFLAGS+=" -DCONFIG_OSDP_SKIP_MARK_BYTE"
fi

if [[ ! -z "${STATIC_PD}" ]]; then
	CCFLAGS+=" -DCONFIG_OSDP_STATIC_PD"
fi

if [[ ! -z "${DEBUG}" ]]; then
	CCFLAGS+=" -g"
fi

## Repo meta data
echo "Extracting source code meta information"
PROJECT_VERSION=$(perl -ne 'print if s/^project\(libosdp VERSION ([0-9.]+)\)$/\1/' CMakeLists.txt)
if [[ -z "${PROJECT_VERSION}" ]]; then
	echo "Failed to parse PROJECT_VERSION!"
	exit -1
fi

PROJECT_NAME=libosdp
if [[ -d .git ]]; then
	GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
	GIT_REV=$(git log --pretty=format:'%h' -n 1)
	GIT_TAG=$(git describe --exact-match --tags 2>/dev/null)
	GIT_DIFF=$(git diff --quiet --exit-code || echo +)
fi

if [[ "${CRYPTO}" == "openssl" ]]; then
	LIBOSDP_SOURCES+=" src/crypto/openssl.c"
elif [[ "${CRYPTO}" == "mbedtls" ]]; then
	LIBOSDP_SOURCES+=" src/crypto/mbedtls.c"
	LDFLAGS+=" -lmbedcrypto -lmbedtls"
else
	echo "Using in-tree AES methods. Consider using openssl/mbedtls (see --crypto)"
	LIBOSDP_SOURCES+=" src/crypto/tinyaes_src.c src/crypto/tinyaes.c"
fi

if [[ ! -z "${CRYPTO_INCLUDE_DIR}" ]]; then
	CCFLAGS+=" -I${CRYPTO_INCLUDE_DIR}"
	CXXFLAGS+=" -I${CRYPTO_INCLUDE_DIR}"
fi

if [[ ! -z "${CRYPTO_LD_FLAGS}" ]]; then
	LDFLAGS+=" -I${CRYPTO_LD_FLAGS}"
fi

## Declare sources
LIBOSDP_SOURCES+=" src/osdp_common.c src/osdp_phy.c src/osdp_sc.c src/osdp_file.c src/osdp_pd.c"
LIBOSDP_SOURCES+=" utils/src/list.c utils/src/queue.c utils/src/slab.c utils/src/utils.c"
LIBOSDP_SOURCES+=" utils/src/disjoint_set.c utils/src/logger.c"

if [[ -z "${STATIC_PD}" ]]; then
	LIBOSDP_SOURCES+=" src/osdp_cp.c"
	TARGETS="cp_app pd_app"
else
	TARGETS="pd_app"
fi

OSDPCTL_SOURCES="osdpctl/ini_parser.c osdpctl/config.c osdpctl/arg_parser.c"
OSDPCTL_SOURCES+=" osdpctl/osdpctl.c osdpctl/cmd_start.c osdpctl/cmd_send.c"
OSDPCTL_SOURCES+=" osdpctl/cmd_others.c"
TARGETS+=" osdpctl"

TEST_SOURCES="tests/unit-tests/test.c tests/unit-tests/test-cp-phy.c tests/unit-tests/test-cp-phy-fsm.c"
TEST_SOURCES+=" tests/unit-tests/test-cp-fsm.c tests/unit-tests/test-file.c"
TEST_SOURCES+=" ${LIBOSDP_SOURCES} utils/src/workqueue.c utils/src/circbuf.c"
TEST_SOURCES+=" utils/src/event.c utils/src/fdutils.c"

if [[ ! -z "${LIB_ONLY}" ]]; then
	TARGETS=""
fi

## Generate osdp_config.h
echo "Generating osdp_config.h"
CONFIG_OUT=${BUILD_DIR}/osdp_config.h
cp src/osdp_config.h.in ${CONFIG_OUT}
sed -i "" -e "s/@PROJECT_VERSION@/${PROJECT_VERSION}/" ${CONFIG_OUT}
sed -i "" -e "s/@PROJECT_NAME@/${PROJECT_NAME}/" ${CONFIG_OUT}
sed -i "" -e "s/@GIT_BRANCH@/${GIT_BRANCH}/" ${CONFIG_OUT}
sed -i "" -e "s/@GIT_REV@/${GIT_REV}/" ${CONFIG_OUT}
sed -i "" -e "s/@GIT_TAG@/${GIT_TAG}/" ${CONFIG_OUT}
sed -i "" -e "s/@GIT_DIFF@/${GIT_DIFF}/" ${CONFIG_OUT}
sed -i "" -e "s|@REPO_ROOT@|${SCRIPT_DIR}|" ${CONFIG_OUT}

## Generate osdp_exports.h
echo "Generating osdp_exports.h"
cat > ${BUILD_DIR}/osdp_export.h <<----
#ifndef OSDP_EXPORT_H
#define OSDP_EXPORT_H

#define OSDP_EXPORT
#define OSDP_NO_EXPORT
#define OSDP_DEPRECATED_EXPORT

#endif /* OSDP_EXPORT_H */
---

## Generate Makefile
echo "Generating config.make"
cat > config.make << ---
CC=${CC}
CXX=${CXX}
AR=${AR}
SYSROOT=${SYSROOT}
CCFLAGS=${CCFLAGS}
CXXFLAGS=${CXXFLAGS}
LDFLAGS=${LDFLAGS}
SRC_LIBOSDP=${LIBOSDP_SOURCES}
SRC_OSDPCTL=${OSDPCTL_SOURCES}
SRC_TEST=${TEST_SOURCES}
TARGETS=${TARGETS}
BUILD_DIR=$(realpath ${BUILD_DIR})
---
echo
echo "LibOSDP lean build system configured!"
echo "Invoke 'make' to build."
