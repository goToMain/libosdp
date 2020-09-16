#
#  Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(SET CMP0063 NEW)

project(libosdp VERSION 1.3.0)

set(PROJECT_AUTHOR "Siddharth Chandrasekran")
set(PROJECT_AUTHOR_EMAIL "siddharth@embedjournal.com")
set(PROJECT_YEAR "2019")
set(PROJECT_ORG "goToMain")
set(PROJECT_URL "https://github.com/goToMain/libosdp/")
set(PROJECT_HOMEPAGE "https://libosdp.gotomain.io/")
set(PROJECT_DESCRIPTION "Open Supervised Device Protocol (OSDP) Library")
set(PROJECT_LICENSE "Apache-2.0")

## Options
option(CONFIG_OSDP_PACKET_TRACE "Enable raw packet trace for diagnostics" OFF)

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddCCompilerFlag)
include(GitInfo)
include(BuildType)

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)

list(APPEND ADDITIONAL_CLEAN_FILES ${CMAKE_SOURCE_DIR}/include/osdp_config.h)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(utils)
add_subdirectory(src)
add_subdirectory(osdpctl)
add_subdirectory(test)
add_subdirectory(samples/c)
add_subdirectory(doc)
add_subdirectory(python)

## uninstall target
add_custom_target(uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)

## include pacakge rules at last
include(CreatePackages)
