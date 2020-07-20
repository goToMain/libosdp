#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/include/osdp_config.h.in
	${CMAKE_CURRENT_SOURCE_DIR}/include/osdp_config.h @ONLY
)

set(LIB_OSDP osdp)
set(LIB_OSDP_STATIC osdpstatic)

list(APPEND LIB_OSDP_SRC
	osdp_common.c
	osdp_phy.c
	osdp_sc.c
	osdp_aes.c
	osdp_cp.c
	osdp_pd.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${LIB_OSDP} SHARED ${LIB_OSDP_SRC})

set_target_properties(${LIB_OSDP} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP} PARENT_SCOPE)

install(
	TARGETS ${LIB_OSDP}
	DESTINATION ${CMAKE_INSTALL_DIR}/lib
)

install(
	FILES ${CMAKE_SOURCE_DIR}/include/osdp.h
	DESTINATION ${CMAKE_INSTALL_DIR}/include
)

if (CONFIG_OSDP_BUILD_STATIC)
	add_library(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SRC})
	set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP_STATIC} PARENT_SCOPE)
	install(
		TARGETS ${LIB_OSDP_STATIC}
		DESTINATION ${CMAKE_INSTALL_DIR}/lib
	)
endif()
