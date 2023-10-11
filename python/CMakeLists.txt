#
#  Copyright (c) 2020-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

find_package(Python3)
if(Python3_FOUND)

set(OSDP_PYTHON_PACKAGE_NAME osdp)
set(OSDP_PYTHON_PACKAGE_VERSION ${PROJECT_VERSION})

configure_file(setup.py.in setup.py)
configure_file(pyproject.toml pyproject.toml)

add_custom_target(python_build
	COMMAND ${Python3_EXECUTABLE} setup.py build_ext --inplace
)

add_custom_target(python_install
	COMMAND ${Python3_EXECUTABLE} setup.py install --user --prefix=
		--record ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)

add_custom_target(python_clean
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/python
)

add_custom_target(python_uninstall
	COMMAND xargs rm < ${CMAKE_BINARY_DIR}/python_install_manifest.txt
)

add_custom_target(pypi_build
	COMMAND ${Python3_EXECUTABLE} -m build
)

add_custom_target(pypi_upload
	COMMAND ${Python3_EXECUTABLE} -m twine upload --repository testpypi dist/*
)

endif() # Python3_FOUND
