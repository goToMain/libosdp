#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

# Generate osdp_config.h in build dir.
configure_file(include/osdp_config.h.in include/osdp_config.h @ONLY)

# keep all symbols hidden by default (-fvisibility=hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET   hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# create libosdp
set(LIB_OSDP osdp)
list(APPEND LIB_OSDP_SRC
	osdp_common.c
	osdp_phy.c
	osdp_sc.c
	osdp_aes.c
	osdp_cp.c
	osdp_pd.c
)
add_library(${LIB_OSDP} ${LIB_OSDP_SRC})
set_target_properties(${LIB_OSDP} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/osdp.h
)
target_include_directories(${LIB_OSDP}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_BINARY_DIR}/src/include
)

# generate osdp_export.h for OSDP_EXPORT() macro
include(GenerateExportHeader)
generate_export_header(${LIB_OSDP}
	EXPORT_MACRO_NAME OSDP_EXPORT
	EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/src/include/osdp_export.h
)

# Package Configuration
include(CMakePackageConfigHelpers)
set(LIBOSDP_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/libosdp)
set(LIBOSDP_DEFINITIONS      "")
set(LIBOSDP_INCLUDE_DIR      ${CMAKE_INSTALL_PREFIX}/include)
set(LIBOSDP_LIBRARY          ${LIB_OSDP})
set(LIBOSDP_LIBRARY_DIR      ${CMAKE_INSTALL_PREFIX}/lib)
set(LIBOSDP_ROOT_DIR         ${CMAKE_INSTALL_PREFIX})
set(LIBOSDP_VERSION_STRING   ${PROJECT_VERSION})
set(LIBOSDP_VERSION_MAJOR    ${PROJECT_VERSION_MAJOR})
set(LIBOSDP_VERSION_MINOR    ${PROJECT_VERSION_MINOR})
set(LIBOSDP_VERSION_PATCH    ${PROJECT_VERSION_PATCH})
configure_package_config_file(
	${CMAKE_SOURCE_DIR}/cmake/LibOSDPConfig.cmake.in
	${CMAKE_BINARY_DIR}/LibOSDPConfig.cmake
	INSTALL_DESTINATION ${LIBOSDP_CMAKE_CONFIG_DIR}
	PATH_VARS
		LIBOSDP_ROOT_DIR
		LIBOSDP_INCLUDE_DIR
		LIBOSDP_LIBRARY_DIR
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file (
	${CMAKE_BINARY_DIR}/LibOSDPConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

# Install targets
install(
	TARGETS ${LIB_OSDP}
	EXPORT ${LIB_OSDP}-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
	ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
install(
	FILES
		${CMAKE_BINARY_DIR}/LibOSDPConfig.cmake
		${CMAKE_BINARY_DIR}/LibOSDPConfigVersion.cmake
	DESTINATION ${LIBOSDP_CMAKE_CONFIG_DIR}
)
install(
	EXPORT ${LIB_OSDP}-targets
	FILE LibOSDPTargets.cmake
	DESTINATION ${LIBOSDP_CMAKE_CONFIG_DIR}
)
