#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(BIN_OSDPCTL osdpctl)

list(APPEND BIN_OSDPCTL_SRC
	ini_parser.c
	config.c
	arg_parser.c
	utils.c
	serial.c
	channel_uart.c
	channel_msgq.c
	channel_custom.c
	channel.c
	osdpctl.c
	cmd_start.c
	cmd_send.c
	cmd_others.c
)

find_package(LibOSDP NO_MODULE REQUIRED HINTS ${CMAKE_BINARY_DIR})

add_executable(${BIN_OSDPCTL} ${BIN_OSDPCTL_SRC})
target_link_libraries(${BIN_OSDPCTL}
	${LIBOSDP_LIBRARY}
)
target_include_directories(${BIN_OSDPCTL}
	PRIVATE
		${LIBOSDP_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

## Strip binary for release builds
if (CMAKE_BUILD_TYPE STREQUAL Release)
	add_custom_command(TARGET ${BIN_OSDPCTL} POST_BUILD
		COMMAND ${CMAKE_STRIP} "${CMAKE_BINARY_DIR}/bin/${BIN_OSDPCTL}"
	)
endif ()

install(
	TARGETS ${BIN_OSDPCTL}
	DESTINATION "bin"
	COMPONENT distributables
)
