#
#  Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(OSDP_UNIT_TEST osdp_unit_test)

# tests can include private headers
include_directories(${LIB_OSDP_INCLUDE_DIRS})
include_directories(${LIB_OSDP_PRIVATE_INCLUDE_DIRS})

# rebuild libosdp with test flag
set(LIB_OSDP_TEST osdptest)
add_definitions(-DUNIT_TESTING ${LIB_OSDP_DEFINITIONS})
add_library(${LIB_OSDP_TEST} STATIC EXCLUDE_FROM_ALL ${LIB_OSDP_SOURCES})

list(APPEND OSDP_UNIT_TEST_SRC
	test.c
	test-cp-phy.c
	test-cp-phy-fsm.c
	test-cp-fsm.c
	test-mixed-fsm.c
	test-commands.c
)

add_executable(${OSDP_UNIT_TEST} EXCLUDE_FROM_ALL ${OSDP_UNIT_TEST_SRC})

target_link_libraries(${OSDP_UNIT_TEST} ${LIB_OSDP_TEST} ${LIB_OSDP_LIBRARIES})

include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/utils/include
)

add_custom_target(check
	COMMAND ${CMAKE_BINARY_DIR}/bin/${OSDP_UNIT_TEST}
	COMMAND rm ${CMAKE_BINARY_DIR}/bin/${OSDP_UNIT_TEST}
	DEPENDS ${OSDP_UNIT_TEST}
)
