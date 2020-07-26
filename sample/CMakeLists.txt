#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

find_package(LibOSDP NO_MODULE REQUIRED HINTS ${CMAKE_BINARY_DIR})

set(CP_SAMPLE cp_sample)
set(PD_SAMPLE pd_sample)

include_directories(${LIBOSDP_INCLUDE_DIR} ${CMAKE_SOURCE_DIRECTORY}/include)

add_executable(${CP_SAMPLE} cp_app.c)
add_executable(${PD_SAMPLE} pd_app.c)

target_link_libraries(${CP_SAMPLE} ${LIBOSDP_LIBRARY})
target_link_libraries(${PD_SAMPLE} ${LIBOSDP_LIBRARY})
