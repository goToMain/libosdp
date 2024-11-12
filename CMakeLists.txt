#
#  Copyright (c) 2019-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
cmake_policy(SET CMP0063 NEW)

project(libosdp VERSION 3.0.8)

set(PROJECT_AUTHOR       "Siddharth Chandrasekaran")
set(PROJECT_AUTHOR_EMAIL "sidcha.dev@gmail.com")
set(PROJECT_YEAR         "2019")
set(PROJECT_ORG          "goToMain")
set(PROJECT_URL          "https://github.com/goToMain/libosdp/")
set(PROJECT_HOMEPAGE     "https://libosdp.sidcha.dev/")
set(PROJECT_DESCRIPTION  "Open Supervised Device Protocol (OSDP) Library")
set(PROJECT_LICENSE      "Apache License, Version 2.0 (Apache-2.0)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddCCompilerFlag)
include(GitInfo)
include(BuildType)
include(GNUInstallDirs)

## Global settings
if(NOT MSVC)
	add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_WARN_DEPRECATED ON)

option(CONFIG_OSDP_STATIC_PD "Setup PD single statically" OFF)
option(CONFIG_OSDP_LIB_ONLY "Only build the library" OFF)

# Each subdirectory has it's own CMakeLists.txt
include(GitSubmodules)
add_subdirectory(src)
if (NOT CONFIG_OSDP_STATIC_PD AND NOT CONFIG_OSDP_LIB_ONLY AND NOT MSVC)
	add_subdirectory(utils)
	add_subdirectory(tests/unit-tests)
	add_subdirectory(examples/c)
	add_subdirectory(examples/cpp)
	add_subdirectory(doc)
endif()

## uninstall target
add_custom_target(uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)

## include package rules at last
include(CreatePackages)
