#
#  Copyright (c) 2020 Siddharth Chandrasekaran
#
#  SPDX-License-Identifier: Apache-2.0
#

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

add_library(${LIB_OSDP} SHARED ${LIB_OSDP_SRC})

set_target_properties(${LIB_OSDP} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP} PARENT_SCOPE)

if (CONFIG_OSDP_BUILD_STATIC)
	add_library(${LIB_OSDP_STATIC} STATIC ${LIB_OSDP_SRC})
	set(OSDP_LIBRARY_TO_LINK ${LIB_OSDP_STATIC} PARENT_SCOPE)
endif()

install(TARGETS ${LIB_OSDP} DESTINATION ${CMAKE_INSTALL_DIR}/lib)

if (CONFIG_OSDP_BUILD_STATIC)
	install(TARGETS ${LIB_OSDP_STATIC} DESTINATION ${CMAKE_INSTALL_DIR}/lib)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION ${CMAKE_INSTALL_DIR}/include)
