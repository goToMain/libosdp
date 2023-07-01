#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.0)
project(osdp_c_sample)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	# This sample is built individually so try to locate an installed
	# version of libosdp.
	find_package(libosdp NO_MODULE REQUIRED)
endif()

set(CP_SAMPLE c_cp_sample)
set(PD_SAMPLE c_pd_sample)

include_directories(${LIBOSDP_INCLUDE_DIR} ${CMAKE_SOURCE_DIRECTORY}/include)

add_executable(${CP_SAMPLE} cp_app.c)
add_executable(${PD_SAMPLE} pd_app.c)

target_link_libraries(${CP_SAMPLE} osdp)
target_link_libraries(${PD_SAMPLE} osdp)
