#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

# Generate osdp_config.h in build dir and add it to include path.
configure_file( include/osdp_config.h.in include/osdp_config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/src/include)

set(LIB_OSDP osdp)
set(OSDP_LIBRARY ${LIB_OSDP} PARENT_SCOPE)

list(APPEND LIB_OSDP_SRC
	osdp_common.c
	osdp_phy.c
	osdp_sc.c
	osdp_aes.c
	osdp_cp.c
	osdp_pd.c
)

# private header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB LIB_OSDP_PUB_INC ${CMAKE_SOURCE_DIR}/include/*.h)

add_library(${LIB_OSDP} ${LIB_OSDP_SRC})

set_target_properties(${LIB_OSDP} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	PUBLIC_HEADER "${LIB_OSDP_PUB_INC}"
)

install(
	TARGETS ${LIB_OSDP}
	LIBRARY DESTINATION ${CMAKE_INSTALL_DIR}/lib
	ARCHIVE DESTINATION ${CMAKE_INSTALL_DIR}/lib
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_DIR}/include
)
