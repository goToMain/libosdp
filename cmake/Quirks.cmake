#
#  Copyright (c) 2021 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# regular Clang or AppleClang
	add_compile_definitions("__fallthrough=__attribute__((fallthrough))")
endif()

