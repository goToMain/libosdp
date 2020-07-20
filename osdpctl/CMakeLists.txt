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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(${BIN_OSDPCTL} ${BIN_OSDPCTL_SRC})

target_link_libraries(${BIN_OSDPCTL} ${READLINE_LIBRARY} ${OSDP_LIBRARY_TO_LINK})

install(
	TARGETS ${BIN_OSDPCTL}
	DESTINATION ${CMAKE_INSTALL_DIR}/bin
)
