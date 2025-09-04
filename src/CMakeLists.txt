#
#  Copyright (c) 2020-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

if (NOT OPT_BUILD_STATIC AND NOT OPT_BUILD_SHARED)
	message(FATAL_ERROR "Both static and shared builds must not be disabled")
endif()

if (OPT_OSDP_PACKET_TRACE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_PACKET_TRACE")
endif()

if (OPT_OSDP_DATA_TRACE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_DATA_TRACE")
endif()

if (OPT_OSDP_SKIP_MARK_BYTE)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_SKIP_MARK_BYTE")
endif()

if (OPT_DISABLE_PRETTY_LOGGING)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_DISABLE_PRETTY_LOGGING")
endif()

if (OPT_OSDP_STATIC_PD)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_STATIC_PD")
endif()

# optionally, find and use OpenSSL or MbedTLS
find_package(OpenSSL)

if (NOT OpenSSL_FOUND)
	find_package(MbedTLS)
else()
	set(MbedTLS_FOUND FALSE)
endif()

# Generate osdp_config.h in build dir.
set(REPO_ROOT ${CMAKE_SOURCE_DIR})
configure_file(osdp_config.h.in ${PROJECT_BINARY_DIR}/include/osdp_config.h @ONLY)

list(APPEND LIB_OSDP_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_common.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_phy.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_pd.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_sc.c
	${CMAKE_CURRENT_SOURCE_DIR}/osdp_file.c
)

if (NOT OPT_OSDP_STATIC_PD)
	list(APPEND LIB_OSDP_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/osdp_cp.c
	)
endif()

if (OPT_OSDP_PACKET_TRACE OR OPT_OSDP_DATA_TRACE)
	list(APPEND LIB_OSDP_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/osdp_diag.c
	)
endif()

list(APPEND LIB_OSDP_INCLUDE_DIRS
	${PROJECT_BINARY_DIR}/include
)

list(APPEND LIB_OSDP_PRIVATE_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

if (OpenSSL_FOUND)
	list(APPEND LIB_OSDP_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY}) # Needed for python library building
	list(APPEND LIB_OSDP_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_USE_OPENSSL")
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/openssl.c)
elseif (MbedTLS_FOUND)
	list(APPEND LIB_OSDP_DEFINITIONS "-DOPT_OSDP_USE_MBEDTLS")
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/mbedtls.c)
else()
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tinyaes.c)
	list(APPEND LIB_OSDP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tinyaes_src.c)
endif()

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
	${PROJECT_SOURCE_DIR}/utils/src/crc16.c
)

list(APPEND LIB_OSDP_HEADERS
	${PROJECT_SOURCE_DIR}/include/osdp.h
	${PROJECT_SOURCE_DIR}/include/osdp.hpp
	${PROJECT_BINARY_DIR}/include/osdp_export.h
)

# These variables are used in other parts of this projects for rebuilding
# LibOSDP as they see fit. For instance pyosdp is built by setuptools.
set(LIB_OSDP_SOURCES ${LIB_OSDP_SOURCES} PARENT_SCOPE)
set(LIB_OSDP_HEADERS ${LIB_OSDP_HEADERS} PARENT_SCOPE)
set(LIB_OSDP_LIBRARIES ${LIB_OSDP_LIBRARIES} PARENT_SCOPE)
set(LIB_OSDP_INCLUDE_DIRS ${LIB_OSDP_INCLUDE_DIRS} PARENT_SCOPE)
set(LIB_OSDP_PRIVATE_INCLUDE_DIRS ${LIB_OSDP_PRIVATE_INCLUDE_DIRS} PARENT_SCOPE)
set(LIB_OSDP_DEFINITIONS ${LIB_OSDP_DEFINITIONS} PARENT_SCOPE)

add_definitions(${LIB_OSDP_DEFINITIONS})

################################################
##
## Build Static Library (OPT_BUILD_SHARED=on)
##
################################################
if (OPT_BUILD_STATIC)

set(LIB_OSDP_STATIC osdpstatic)
set(LIB_TARGET ${LIB_OSDP_STATIC}) ## to be used in libosdp.pc.in
add_library(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SOURCES} ${LIB_OSDP_UTILS_SRC})
target_link_libraries(${LIB_OSDP_STATIC} ${LIB_OSDP_LIBRARIES})

set_target_properties(${LIB_OSDP_STATIC} PROPERTIES
	VERSION ${PROJECT_VERSION}
	COMPILE_FLAGS -DOSDP_STATIC_DEFINE
	PUBLIC_HEADER "${LIB_OSDP_HEADERS}"
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(${LIB_OSDP_STATIC}
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
		$<INSTALL_INTERFACE:include/libosdp>
	PRIVATE
		${LIB_OSDP_INCLUDE_DIRS}
		${LIB_OSDP_PRIVATE_INCLUDE_DIRS}
		${PROJECT_SOURCE_DIR}/utils/include
)

if (OPT_BUILD_SANITIZER)
	target_compile_options(${LIB_OSDP_STATIC} PRIVATE
		-fsanitize=address,undefined,leak
	)
	target_link_options(${LIB_OSDP_STATIC} PRIVATE
		-fsanitize=address,undefined,leak
	)
endif()

install(
	TARGETS ${LIB_OSDP_STATIC}
	COMPONENT distributables
	EXPORT LibOSDPTargets
	LIBRARY DESTINATION ${LIBOSDP_LIBRARY_DIR}
	ARCHIVE DESTINATION ${LIBOSDP_LIBRARY_DIR}
	PUBLIC_HEADER DESTINATION include/libosdp
)

endif() # OPT_BUILD_STATIC

################################################
##
## Build Shared Library (OPT_BUILD_SHARED=on)
##
################################################
if (OPT_BUILD_SHARED)

set(LIB_OSDP_SHARED osdp)
set(LIB_TARGET ${LIB_OSDP_SHARED}) ## to be used in libosdp.pc.in
# keep all symbols hidden by default (-fvisibility=hidden) for shared library
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (OPT_OSDP_PACKET_TRACE OR OPT_OSDP_DATA_TRACE)
	list(APPEND LIB_OSDP_UTILS_SRC
		${PROJECT_SOURCE_DIR}/utils/src/pcap_gen.c
	)
endif()

set(LIB_OSDP_UTILS_SRC ${LIB_OSDP_UTILS_SRC} PARENT_SCOPE)

add_library(${LIB_OSDP_SHARED} SHARED ${LIB_OSDP_SOURCES} ${LIB_OSDP_UTILS_SRC})
if (OpenSSL_FOUND)
	target_link_libraries(${LIB_OSDP_SHARED} PUBLIC OpenSSL::Crypto)
elseif (MbedTLS_FOUND)
	target_link_libraries(${LIB_OSDP_SHARED} PUBLIC MbedTLS::mbedcrypto)
endif()

set_target_properties(${LIB_OSDP_SHARED} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	PUBLIC_HEADER "${LIB_OSDP_HEADERS}"
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(${LIB_OSDP_SHARED}
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
		$<INSTALL_INTERFACE:include/libosdp>
	PRIVATE
		${LIB_OSDP_PRIVATE_INCLUDE_DIRS}
		${PROJECT_SOURCE_DIR}/utils/include
)

if (OPT_BUILD_SANITIZER)
	target_compile_options(${LIB_OSDP_SHARED} PRIVATE
		-fsanitize=address,undefined,leak
	)
	target_link_options(${LIB_OSDP_SHARED} PRIVATE
		-fsanitize=address,undefined,leak
	)
endif()

install(
	TARGETS ${LIB_OSDP_SHARED}
	EXPORT LibOSDPTargets
	LIBRARY
		COMPONENT distributables
		DESTINATION ${LIBOSDP_LIBRARY_DIR}
	ARCHIVE
		COMPONENT distributables
		DESTINATION ${LIBOSDP_LIBRARY_DIR}
	PUBLIC_HEADER
		COMPONENT headers
		DESTINATION include/libosdp
)

endif() # OPT_BUILD_SHARED

install(FILES ${PROJECT_BINARY_DIR}/include/osdp_export.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libosdp
	COMPONENT headers
)

if (NOT OPT_BUILD_BARE_METAL)

# pkg-config file
configure_file(
	${PROJECT_SOURCE_DIR}/misc/libosdp.pc.in
	${CMAKE_BINARY_DIR}/libosdp.pc @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/libosdp.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	COMPONENT config_files
)

## Package Configuration

include(CMakePackageConfigHelpers)
install(EXPORT LibOSDPTargets
	FILE LibOSDPTargets.cmake
	NAMESPACE libosdp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libosdp
	COMPONENT config_files
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
	COMPONENT config_files
)

endif() # NOT OPT_BUILD_BARE_METAL
