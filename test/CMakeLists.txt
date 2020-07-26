#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(OSDP_UNIT_TEST osdp_unit_test)

# tests can include private headers
include_directories(${CMAKE_SOURCE_DIR}/src/include) # for osdp_common.h
include_directories(${CMAKE_BINARY_DIR}/src/include) # for osdp_config.h

# rebuild libosdp with test flag
set(LIB_OSDP_TEST osdptest)
list(APPEND LIB_OSDP_TEST_SRC
	${CMAKE_SOURCE_DIR}/src/osdp_common.c
	${CMAKE_SOURCE_DIR}/src/osdp_phy.c
	${CMAKE_SOURCE_DIR}/src/osdp_sc.c
	${CMAKE_SOURCE_DIR}/src/osdp_aes.c
	${CMAKE_SOURCE_DIR}/src/osdp_cp.c
	${CMAKE_SOURCE_DIR}/src/osdp_pd.c
)
add_definitions(-DUNIT_TESTING)
add_library(${LIB_OSDP_TEST} STATIC EXCLUDE_FROM_ALL ${LIB_OSDP_TEST_SRC})

list(APPEND OSDP_UNIT_TEST_SRC
	test.c
	test-cp-phy.c
	test-cp-phy-fsm.c
	test-cp-fsm.c
	test-mixed-fsm.c
)

add_executable(${OSDP_UNIT_TEST} EXCLUDE_FROM_ALL ${OSDP_UNIT_TEST_SRC})

target_link_libraries(${OSDP_UNIT_TEST} ${LIB_OSDP_TEST})

add_custom_target(check
	COMMAND ${CMAKE_BINARY_DIR}/bin/${OSDP_UNIT_TEST}
	COMMAND rm ${CMAKE_BINARY_DIR}/bin/${OSDP_UNIT_TEST}
	DEPENDS ${OSDP_UNIT_TEST}
)
