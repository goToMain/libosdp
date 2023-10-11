#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

add_custom_target(check
    COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/python/ python3 -m pytest -vv --show-capture=no
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running tests.."
)

add_dependencies(check python_build)