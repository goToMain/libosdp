#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(LIB_OSDP_TEST osdp-test)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

list(APPEND LIB_OSDP_TEST_SRC
	test.c
	test-cp-phy.c
	test-cp-phy-fsm.c
	test-cp-fsm.c
	test-mixed-fsm.c
)

add_executable(${LIB_OSDP_TEST} EXCLUDE_FROM_ALL ${LIB_OSDP_TEST_SRC})

target_link_libraries(${LIB_OSDP_TEST} osdp)

add_custom_target(check
	COMMAND ${CMAKE_BINARY_DIR}/bin/${LIB_OSDP_TEST}
	COMMAND rm ${CMAKE_BINARY_DIR}/bin/${LIB_OSDP_TEST}
	DEPENDS ${LIB_OSDP_TEST}
)
