#
#  Copyright (c) 2020 Siddharth Chandrasekaran
#
#  SPDX-License-Identifier: Apache-2.0
#

set(CP_SAMPLE cp_sample)
set(PD_SAMPLE pd_sample)

add_executable(${CP_SAMPLE} cp_app.c)
add_executable(${PD_SAMPLE} pd_app.c)

target_link_libraries(${CP_SAMPLE} ${OSDP_LIBRARY_TO_LINK})
target_link_libraries(${PD_SAMPLE} ${OSDP_LIBRARY_TO_LINK})
