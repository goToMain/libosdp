#
#  Copyright (c) 2019 Siddharth Chandrasekaran
#
#  SPDX-License-Identifier: MIT
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
	"src/osdp_common.c"
	"src/osdp_phy.c"
	"src/osdp_sc.c"
	"src/aes.c"
	"src/osdp_cp_phy.c"
	"src/osdp_cp_fsm.c"
	"src/osdp_cp.c"
	"src/osdp_pd.c"
)
ADD_LIBRARY(${LIB_OSDP} STATIC ${LIB_OSDP_SRC})

## tests
set(LIB_OSDP_TEST osdp-test)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND LIB_OSDP_TEST_SRC
	"test/test.c"
	"test/test-cp-phy.c"
	"test/test-cp-phy-fsm.c"
	"test/test-cp-fsm.c"
	"test/test-mixed-fsm.c"
)
add_executable(${LIB_OSDP_TEST} ${LIB_OSDP_TEST_SRC})
target_link_libraries(${LIB_OSDP_TEST} osdp)

## Sample CP
set(LIB_OSDP_SAMPLE_CP osdp-cp-sample)
list(APPEND LIB_OSDP_SAMPLE_CP_SRC
	"sample/rs232.c"
	"sample/uart.c"
	"sample/cp_app.c"
)
add_executable(${LIB_OSDP_SAMPLE_CP} ${LIB_OSDP_SAMPLE_CP_SRC})
target_link_libraries(${LIB_OSDP_SAMPLE_CP} osdp)

## Install
set(CMAKE_INSTALL_DIR $ENV{INSTALL_PREFIX})
install(TARGETS ${LIB_OSDP} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/osdp.h DESTINATION ${CMAKE_INSTALL_DIR}/include)
