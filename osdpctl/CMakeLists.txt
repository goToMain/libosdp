#
#  Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(BIN_OSDPCTL osdpctl)

list(APPEND BIN_OSDPCTL_SRC
	ini_parser.c
	config.c
	arg_parser.c
	osdpctl.c
	cmd_start.c
	cmd_send.c
	cmd_others.c
)

find_package(LibOSDP NO_MODULE REQUIRED HINTS ${CMAKE_BINARY_DIR})

add_executable(${BIN_OSDPCTL} ${BIN_OSDPCTL_SRC})
target_link_libraries(${BIN_OSDPCTL} ${LIBOSDP_LIBRARY} utils)
target_include_directories(${BIN_OSDPCTL}
	PUBLIC
		${CMAKE_SOURCE_DIR}/utils/include
		${LIBOSDP_INCLUDE_DIR}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
	TARGETS ${BIN_OSDPCTL}
	DESTINATION "bin"
	COMPONENT distributables
)
