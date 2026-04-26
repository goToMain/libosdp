#
#  Copyright (c) 2020-2026 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(OSDP_UNIT_TEST osdp_unit_test)

# tests can include private headers
include_directories(${LIB_OSDP_INCLUDE_DIRS})
include_directories(${LIB_OSDP_PRIVATE_INCLUDE_DIRS})

# rebuild libosdp with test flag. Shorten the PD online timeout so
# link-loss tests do not block for the production 8s window.
set(LIB_OSDP_TEST osdptest)
add_definitions(-DUNIT_TESTING -DOSDP_PD_ONLINE_TOUT_MS=1500
		${LIB_OSDP_DEFINITIONS})
add_library(${LIB_OSDP_TEST} STATIC EXCLUDE_FROM_ALL
	    ${LIB_OSDP_SOURCES} ${PROJECT_SOURCE_DIR}/src/osdp_file.c)

list(APPEND OSDP_UNIT_TEST_SRC
	test.c
	test-cp-phy.c
	test-pd-phy.c
	test-cp-fsm.c
	test-file.c
	test-commands.c
	test-events.c
	test-hotplug.c
	test-notifications.c
	test-async-fuzz.c
	test-sc.c
	test-sc-sia-vectors.c
)

add_executable(${OSDP_UNIT_TEST} EXCLUDE_FROM_ALL ${OSDP_UNIT_TEST_SRC})

target_link_libraries(${OSDP_UNIT_TEST} ${LIB_OSDP_TEST} osdp utils pthread)

if (OPT_BUILD_SANITIZER)
	target_compile_options(${OSDP_UNIT_TEST} PRIVATE
		-fsanitize=address,undefined,leak
	)
	target_link_options(${OSDP_UNIT_TEST} PRIVATE
		-fsanitize=address,undefined,leak
	)
endif()

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/utils/include
)

# CTest integration: lets `ctest` (and IDEs) discover the unit tests.
# osdp_unit_test is EXCLUDE_FROM_ALL so a plain `cmake --build` does not
# produce the binary. A fixture builds it on demand when ctest runs.
add_test(NAME osdp_unit_test_build
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
		--target ${OSDP_UNIT_TEST} --config $<CONFIG>
)
set_tests_properties(osdp_unit_test_build PROPERTIES
	FIXTURES_SETUP osdp_unit_test_built
)
add_test(NAME osdp_unit_test
	COMMAND ${OSDP_UNIT_TEST}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
set_tests_properties(osdp_unit_test PROPERTIES
	FIXTURES_REQUIRED osdp_unit_test_built
)

# `check` and `check-ut` remain as convenience wrappers around ctest so
# existing docs and CI continue to work unchanged.
add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R osdp_unit_test
	DEPENDS ${OSDP_UNIT_TEST}
	USES_TERMINAL
)
add_custom_target(check-ut
	DEPENDS check
)
