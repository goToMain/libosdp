#
#  Copyright (c) 2019 Siddharth Chandrasekaran
#
#  SPDX-License-Identifier: Apache-2.0
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)
project(libosdp)
set(PROJECT_AUTHOR "Siddharth Chandrasekran")
set(PROJECT_YEAR 2019)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Custom command produces build_meta_data.c, and a dummy output that's
# not actually produced, in order to force Version.cmake to always be
# re-run before the build
ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/build_meta_data.c
		${CMAKE_CURRENT_BINARY_DIR}/_build_meta_data.c
	COMMAND ${CMAKE_COMMAND} -P
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildMetaData.cmake
	COMMENT "Generating build meta data.."
)

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddCCompilerFlag)
include(BuildMetaData)

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)
add_c_compiler_flag(-s RELEASE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

## build libosdp
set(LIB_OSDP osdp)
set(LIB_OSDP_STATIC osdpstatic)
list(APPEND LIB_OSDP_SRC
	"${CMAKE_BINARY_DIR}/build_meta_data.c"
	"src/osdp_common.c"
	"src/osdp_phy.c"
	"src/osdp_sc.c"
	"src/osdp_aes.c"
	"src/osdp_cp.c"
	"src/osdp_pd.c"
)
ADD_LIBRARY(${LIB_OSDP} SHARED ${LIB_OSDP_SRC})
ADD_LIBRARY(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SRC})

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
	DEPENDS ${LIB_OSDP_TEST}
)

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
install(TARGETS ${LIB_OSDP_STATIC} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/osdp.h DESTINATION ${CMAKE_INSTALL_DIR}/include)
