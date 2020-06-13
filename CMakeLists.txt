#
#  Copyright (c) 2019 Siddharth Chandrasekaran
#
#  SPDX-License-Identifier: Apache-2.0
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)

project(libosdp VERSION 1.0.0 LANGUAGES C)

set(PROJECT_AUTHOR "Siddharth Chandrasekran")
set(PROJECT_YEAR 2019)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

## Options
option(CONFIG_OSDP_BUILD_STATIC "Build static archive (in addition to shared library)" OFF)
option(CONFIG_OSDP_PACKET_TRACE "Enable raw packet trace for diagnostics" OFF)

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddCCompilerFlag)
include(GitInfo)

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)
add_c_compiler_flag(-s RELEASE)

## Macro Switches
if (CONFIG_OSDP_PACKET_TRACE)
	add_compile_definitions(OSDP_PACKET_TRACE)
endif()

configure_file(
	${CMAKE_SOURCE_DIR}/include/osdp_config.h.in
	${CMAKE_SOURCE_DIR}/include/osdp_config.h @ONLY
)
list(APPEND ADDITIONAL_CLEAN_FILES ${CMAKE_SOURCE_DIR}/include/osdp_config.h)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_DIR $ENV{INSTALL_PREFIX})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(osdpctl)
add_subdirectory(test)
add_subdirectory(sample)
