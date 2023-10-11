#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.0)
project(osdp_cpp_sample)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	# This sample is built individually so try to locate an installed
	# version of libosdp.
	find_package(libosdp NO_MODULE REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CP_SAMPLE cpp_cp_sample)
set(PD_SAMPLE cpp_pd_sample)

add_executable(${CP_SAMPLE} cp_app.cpp)
add_executable(${PD_SAMPLE} pd_app.cpp)

target_link_libraries(${CP_SAMPLE} osdp)
target_link_libraries(${PD_SAMPLE} osdp)
