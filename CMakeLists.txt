#
#  Copyright (c) 2019-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(SET CMP0063 NEW)

project(libosdp VERSION 1.5.0)

set(PROJECT_AUTHOR       "Siddharth Chandrasekaran")
set(PROJECT_AUTHOR_EMAIL "siddharth@embedjournal.com")
set(PROJECT_YEAR         "2019")
set(PROJECT_ORG          "goToMain")
set(PROJECT_URL          "https://github.com/goToMain/libosdp/")
set(PROJECT_HOMEPAGE     "https://libosdp.gotomain.io/")
set(PROJECT_DESCRIPTION  "Open Supervised Device Protocol (OSDP) Library")
set(PROJECT_LICENSE      "Apache-2.0")

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddCCompilerFlag)
include(GitInfo)
include(BuildType)
include(GNUInstallDirs)

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(CONFIG_OSDP_STATIC_PD "Setup PD single statically" OFF)

# Each subdirecttory has it's own CMakeLists.txt
add_subdirectory(utils)
add_subdirectory(src)
if (NOT CONFIG_OSDP_STATIC_PD)
	add_subdirectory(osdpctl)
	add_subdirectory(test)
	add_subdirectory(samples/c)
	add_subdirectory(samples/cpp)
	add_subdirectory(doc)
	add_subdirectory(python)
endif()

## uninstall target
add_custom_target(uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)

## include pacakge rules at last
include(CreatePackages)
