#
#  SPDX-License-Identifier: MIT
#
#  Author: Siddharth Chandrasekaran
#  Date: Sat Sep 14 20:03:43 IST 2019
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

## Project
project(libosdp)
set(PROJECT_AUTHOR "Siddharth Chandrasekran")
set(PROJECT_YEAR 2019)

## Includes
include(AddCCompilerFlag)

## Global settings
add_c_compiler_flag(-Wall)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

## build libosdp.a
set(LIB_OSDP osdp)
list(APPEND LIB_OSDP_SRC
        "src/common.c"
        "src/cp-fsm.c"
        "src/cp-phy.c"
        "src/osdp-cp.c"
)
ADD_LIBRARY(${LIB_OSDP} STATIC ${LIB_OSDP_SRC})

## tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND LIB_OSDP_TEST_SRC
        "test/test.c"
        "test/test-cp-phy.c"
)
add_executable(test ${LIB_OSDP_TEST_SRC})
target_link_libraries(test osdp)

## Install
set(CMAKE_INSTALL_DIR $ENV{INSTALL_PREFIX})
install(TARGETS ${LIB_OSDP} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/osdp.h DESTINATION ${CMAKE_INSTALL_DIR}/include)
