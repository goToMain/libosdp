#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

find_package(PythonInterp 3 REQUIRED)
find_package(PythonLibs   3 REQUIRED)

set(OSDP_PYTHON_PACKAGE_NAME osdp)
set(OSDP_PYTHON_PACKAGE_VERSION ${PROJECT_VERSION})

configure_file(setup.py.in setup.py)

add_custom_target(python
	COMMAND ${PYTHON_EXECUTABLE} setup.py build
)

add_custom_target(python_install
	COMMAND ${PYTHON_EXECUTABLE} setup.py install
		--record ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)

add_custom_target(python_clean
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/python
)

add_custom_target(python_uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)
