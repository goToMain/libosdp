find_package(Sphinx)
find_package(Doxygen)

if(SPHINX_FOUND AND DOXYGEN_FOUND)

# Find all the public headers
get_target_property(LIBOSDP_PUBLIC_HEADER_DIR osdp INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE LIBOSDP_PUBLIC_HEADERS ${LIBOSDP_PUBLIC_HEADER_DIR}/*.h)

set(DOXYGEN_INPUT_DIRS "${CMAKE_SOURCE_DIR}/include/osdp.h ${CMAKE_SOURCE_DIR}/src")
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)

# Generate Doxyfile in build dir.
configure_file(Doxyfile.in Doxyfile @ONLY)

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${LIBOSDP_PUBLIC_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating doxygen docs")

add_custom_target(Doxygen DEPENDS ${DOXYGEN_INDEX_FILE})

# Generate conf.py in build dir.
configure_file(conf.py.in conf.py @ONLY)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(html_docs
	COMMAND ${SPHINX_EXECUTABLE} -qn -b html
		-c ${CMAKE_CURRENT_BINARY_DIR}
		# Tell Breathe where to find the Doxygen output
		-Dbreathe_projects.LibOSDP=${DOXYGEN_OUTPUT_DIR}/xml
		${SPHINX_SOURCE} ${SPHINX_BUILD}
	WORKING_DIRECTORY ${SPHINX_SOURCE}
	DEPENDS Doxygen
	COMMENT "Generating documentation with Sphinx"
)

endif() # SPHINX_FOUND
