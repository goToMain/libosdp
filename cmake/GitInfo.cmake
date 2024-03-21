#
#  Copyright (c) 2020-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

execute_process(
	COMMAND git log --pretty=format:'%h' -n 1
	OUTPUT_VARIABLE GIT_REV
	ERROR_QUIET
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if ("${GIT_REV}" STREQUAL "")
	set(GIT_REV "")
	set(GIT_DIFF "")
	set(GIT_TAG "")
	set(GIT_BRANCH "None")
else()
	execute_process(
		COMMAND git diff --quiet --exit-code
		RESULT_VARIABLE RETURN_CODE
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
	if(RETURN_CODE AND NOT RETURN_CODE EQUAL 0)
		set(GIT_DIFF "+")
	endif()
	execute_process(
		COMMAND git describe --exact-match --tags
		OUTPUT_VARIABLE GIT_TAG ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND git rev-parse --abbrev-ref HEAD
		OUTPUT_VARIABLE GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
endif()
