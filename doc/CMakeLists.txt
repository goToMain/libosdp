find_package(Sphinx REQUIRED)

# Generate osdp_config.h in build dir.
configure_file(conf.py.in conf.py @ONLY)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(html_docs
	COMMAND ${SPHINX_EXECUTABLE} -aqnW -b html -c ${CMAKE_CURRENT_BINARY_DIR} ${SPHINX_SOURCE} ${SPHINX_BUILD}
	WORKING_DIRECTORY ${SPHINX_SOURCE}
	COMMENT "Generating documentation with Sphinx"
)
