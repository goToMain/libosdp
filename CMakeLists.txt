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

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)
add_c_compiler_flag(-s RELEASE)

## Add compile time defines
add_c_compiler_flag(-DPROJECT_VERSION=\\"${PROJECT_VERSION}\\")
add_c_compiler_flag(-DPROJECT_NAME=\\"${PROJECT_NAME}\\")

## Macro Switches
if (CONFIG_OSDP_PACKET_TRACE)
	add_compile_definitions(OSDP_PACKET_TRACE)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

## build libosdp
set(LIB_OSDP osdp)
set(LIB_OSDP_STATIC osdpstatic)
list(APPEND LIB_OSDP_SRC
	"src/osdp_common.c"
	"src/osdp_phy.c"
	"src/osdp_sc.c"
	"src/osdp_aes.c"
	"src/osdp_cp.c"
	"src/osdp_pd.c"
)
add_library(${LIB_OSDP} SHARED ${LIB_OSDP_SRC})
set_target_properties(${LIB_OSDP} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)
set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP})
if (CONFIG_OSDP_BUILD_STATIC)
	add_library(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SRC})
	set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP_STATIC})
endif()

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
add_executable(${LIB_OSDP_TEST} EXCLUDE_FROM_ALL ${LIB_OSDP_TEST_SRC})
target_link_libraries(${LIB_OSDP_TEST} osdp)
add_custom_target(check
	COMMAND ${CMAKE_BINARY_DIR}/${LIB_OSDP_TEST}
	COMMAND rm ${CMAKE_BINARY_DIR}/${LIB_OSDP_TEST}
	DEPENDS ${LIB_OSDP_TEST}
)

## osdpctl
set(BIN_OSDPCTL osdpctl)
list(APPEND BIN_OSDPCTL_SRC
	"osdpctl/ini_parser.c"
	"osdpctl/config.c"
	"osdpctl/arg_parser.c"
	"osdpctl/utils.c"
	"osdpctl/rs232.c"
	"osdpctl/channel_uart.c"
	"osdpctl/channel_msgq.c"
	"osdpctl/channel_custom.c"
	"osdpctl/channel.c"
	"osdpctl/osdpctl.c"
	"osdpctl/cmd_start.c"
	"osdpctl/cmd_send.c"
	"osdpctl/cmd_others.c"
)
add_executable(${BIN_OSDPCTL} ${BIN_OSDPCTL_SRC})
target_link_libraries(${BIN_OSDPCTL} ${READLINE_LIBRARY} ${OSDP_LIBRARY_TO_LINK})

## Install
set(CMAKE_INSTALL_DIR $ENV{INSTALL_PREFIX})
install(TARGETS ${LIB_OSDP} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
if (CONFIG_OSDP_BUILD_STATIC)
	install(TARGETS ${LIB_OSDP_STATIC} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/osdp.h DESTINATION ${CMAKE_INSTALL_DIR}/include)
