#
#  Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.0)
project(osdp_cpp_sample)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	# This sample is built individually so try to locate an installed
	# version of libosdp.
	find_package(LibOSDP NO_MODULE REQUIRED)
	include(${LIBOSDP_USE_FILE})
else()
	# sample is build from within libosdp as a subdir so try to look
	# inside build dirs
	find_package(LibOSDP NO_MODULE REQUIRED HINTS ${CMAKE_BINARY_DIR})

	add_definitions(${LIBOSDP_DEFINITIONS})
	include_directories(${LIBOSDP_INCLUDE_DIR})
	link_directories(${LIBOSDP_LIBRARY_DIR})
endif()

set(CMAKE_CXX_STANDARD 11)
set(CP_SAMPLE cpp_cp_sample)
set(PD_SAMPLE cpp_pd_sample)

include_directories(${LIBOSDP_INCLUDE_DIR} ${CMAKE_SOURCE_DIRECTORY}/include)

add_executable(${CP_SAMPLE} cp_app.cpp)
add_executable(${PD_SAMPLE} pd_app.cpp)

target_link_libraries(${CP_SAMPLE} ${LIBOSDP_LIBRARY})
target_link_libraries(${PD_SAMPLE} ${LIBOSDP_LIBRARY})
