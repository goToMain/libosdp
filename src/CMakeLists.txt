#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

## Options
option(CONFIG_OSDP_PACKET_TRACE "Enable raw packet trace for diagnostics" OFF)
option(CONFIG_OSDP_DATA_TRACE "Enable command/reply data buffer tracing" OFF)
option(CONFIG_OSDP_SKIP_MARK_BYTE "Don't send the leading mark byte (0xFF)" OFF)
option(CONFIG_DISABLE_PRETTY_LOGGING "Don't colorize log ouputs" OFF)
option(CONFIG_BUILD_ASAN "Address sanitizer enabled builds" OFF)

if (CONFIG_OSDP_PACKET_TRACE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_PACKET_TRACE")
endif()

if (CONFIG_OSDP_DATA_TRACE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_DATA_TRACE")
endif()

if (CONFIG_OSDP_SKIP_MARK_BYTE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_SKIP_MARK_BYTE")
endif()

if (CONFIG_DISABLE_PRETTY_LOGGING)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_DISABLE_PRETTY_LOGGING")
endif()

if (CONFIG_OSDP_STATIC_PD)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_STATIC_PD")
endif()

# static and shared library names
set(LIB_OSDP_SHARED osdp)
set(LIB_OSDP_STATIC osdpstatic)

# optionally, find and use OpenSSL or MbedTLS
find_package(OpenSSL)

if (NOT OpenSSL_FOUND)
	find_package(MbedTLS)
else()
	set(MbedTLS_FOUND FALSE)
endif()

# Generate osdp_config.h in build dir.
set(REPO_ROOT ${CMAKE_SOURCE_DIR})
configure_file(osdp_config.h.in osdp_config.h @ONLY)

list(APPEND LIB_OSDP_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_common.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_phy.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_pd.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_sc.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_file.c
)

if (NOT CONFIG_OSDP_STATIC_PD)
	list(APPEND LIB_OSDP_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/osdp_cp.c
	)
endif()

list(APPEND LIB_OSDP_PRIVATE_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

if (OpenSSL_FOUND)
	list(APPEND LIB_OSDP_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY}) # Needed for python library building
	list(APPEND LIB_OSDP_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_USE_OPENSSL")
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/openssl.c)
elseif (MbedTLS_FOUND)
	list(APPEND LIB_OSDP_DEFINITIONS "-DCONFIG_OSDP_USE_MBEDTLS")
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/mbedtls.c)
else()
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tinyaes.c)
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tinyaes_src.c)
endif()

# These variables are used in other parts of this projects for rebuilding
# LibOSDP as they see fit. For instance pyosdp is built by setuptools.
set(LIB_OSDP_SOURCES ${LIB_OSDP_SOURCES} PARENT_SCOPE)
set(LIB_OSDP_LIBRARIES ${LIB_OSDP_LIBRARIES} PARENT_SCOPE)
set(LIB_OSDP_INCLUDE_DIRS ${LIB_OSDP_INCLUDE_DIRS} PARENT_SCOPE)
set(LIB_OSDP_PRIVATE_INCLUDE_DIRS ${LIB_OSDP_PRIVATE_INCLUDE_DIRS} PARENT_SCOPE)
set(LIB_OSDP_DEFINITIONS ${LIB_OSDP_DEFINITIONS} PARENT_SCOPE)

add_definitions(${LIB_OSDP_DEFINITIONS})

## build libosdpstatic.a

add_library(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SOURCES})
set_target_properties(${LIB_OSDP_STATIC} PROPERTIES COMPILE_FLAGS -DOSDP_STATIC_DEFINE)
target_link_libraries(${LIB_OSDP_STATIC} ${LIB_OSDP_LIBRARIES})

target_include_directories(${LIB_OSDP_STATIC}
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include/libosdp>
	PRIVATE
		${LIB_OSDP_INCLUDE_DIRS}
		${LIB_OSDP_PRIVATE_INCLUDE_DIRS}
		${PROJECT_SOURCE_DIR}/utils/include
)

## build libosdp.so

# keep all symbols hidden by default (-fvisibility=hidden) for shared library
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# For shared library (gcc/linux), utils must be recompiled with -fPIC. Right
# now cmake doesn't support `--whole-archvive ... --no-whole-archive` directive
# to linker (see https://gitlab.kitware.com/cmake/cmake/-/issues/20078).
#
# Note: Mac seems to be impervious to this issue, I suppose it treats all
# objects relocatable by default (?).
#
list(APPEND LIB_OSDP_UTILS_SRC
	${PROJECT_SOURCE_DIR}/utils/src/utils.c
	${PROJECT_SOURCE_DIR}/utils/src/list.c
	${PROJECT_SOURCE_DIR}/utils/src/queue.c
	${PROJECT_SOURCE_DIR}/utils/src/slab.c
	${PROJECT_SOURCE_DIR}/utils/src/disjoint_set.c
	${PROJECT_SOURCE_DIR}/utils/src/logger.c
)
set(LIB_OSDP_UTILS_SRC ${LIB_OSDP_UTILS_SRC} PARENT_SCOPE)

# add source files of utils instead of linking it. See comment above.
add_library(${LIB_OSDP_SHARED} SHARED ${LIB_OSDP_SOURCES} ${LIB_OSDP_UTILS_SRC})
if (OpenSSL_FOUND)
	target_link_libraries(${LIB_OSDP_SHARED} PUBLIC OpenSSL::Crypto)
elseif (MbedTLS_FOUND)
	target_link_libraries(${LIB_OSDP_SHARED} PUBLIC MbedTLS::mbedcrypto)
endif()

set_target_properties(${LIB_OSDP_SHARED} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/osdp.h
)

target_include_directories(${LIB_OSDP_SHARED}
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include/libosdp>
	PRIVATE
		${LIB_OSDP_PRIVATE_INCLUDE_DIRS}
		${PROJECT_SOURCE_DIR}/utils/include
)

# generate osdp_export.h for OSDP_EXPORT() macro
# must be placed after the target definitions.
include(GenerateExportHeader)
generate_export_header(${LIB_OSDP_SHARED}
	BASE_NAME OSDP
	EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/osdp_export.h
)

if (CONFIG_BUILD_ASAN)
	target_compile_options(${LIB_OSDP_SHARED} PRIVATE -fsanitize=address)
	target_compile_options(${LIB_OSDP_STATIC} PRIVATE -fsanitize=address)
	target_link_options(${LIB_OSDP_SHARED} PRIVATE -fsanitize=address)
	target_link_options(${LIB_OSDP_STATIC} PRIVATE -fsanitize=address)
endif()

# pkg-config file
configure_file(
	${PROJECT_SOURCE_DIR}/misc/libosdp.pc.in
	${CMAKE_BINARY_DIR}/libosdp.pc @ONLY
)

## Install targets

install(
	TARGETS ${LIB_OSDP_SHARED} ${LIB_OSDP_STATIC}
	COMPONENT distributables
	EXPORT LibOSDPTargets
	LIBRARY DESTINATION ${LIBOSDP_LIBRARY_DIR}
	ARCHIVE DESTINATION ${LIBOSDP_LIBRARY_DIR}
	PUBLIC_HEADER DESTINATION include/libosdp
)
install(
	FILES
		${CMAKE_BINARY_DIR}/libosdp.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	COMPONENT config_files
)

## Package Configuration

include(CMakePackageConfigHelpers)
install(EXPORT LibOSDPTargets
	FILE LibOSDPTargets.cmake
	NAMESPACE libosdp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libosdp
)
configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/LibOSDPConfig.cmake.in
	${CMAKE_BINARY_DIR}/LibOSDPConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libosdp
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
	INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/LibOSDPConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)
install(FILES
          "${CMAKE_BINARY_DIR}/LibOSDPConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/LibOSDPConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libosdp
)
