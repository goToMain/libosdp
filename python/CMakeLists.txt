#
#  Copyright (c) 2020-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

find_package(PythonInterp 3)
if(PYTHONINTERP_FOUND)
find_package(PythonLibs   3)
if(PYTHONLIBS_FOUND)

set(OSDP_PYTHON_PACKAGE_NAME osdp)
set(OSDP_PYTHON_PACKAGE_VERSION ${PROJECT_VERSION})

configure_file(setup.py.in setup.py)
configure_file(pyproject.toml pyproject.toml)

add_custom_target(python
	COMMAND ${PYTHON_EXECUTABLE} setup.py build
)

add_custom_target(python_install
	COMMAND ${PYTHON_EXECUTABLE} setup.py install --user --prefix=
		--record ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)

add_custom_target(python_clean
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/python
)

add_custom_target(python_uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)

endif() # PYTHONLIBS_FOUND
endif() # PYTHONINTERP_FOUND
