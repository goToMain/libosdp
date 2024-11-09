Build and install
=================

To build libosdp you must have cmake-3.0 (or above) and a C compiler installed.
This repository produces a ``libosdpstatic.a`` and ``libosdp.so``. You can link
these with your application as needed (-losdp or -losdpstatic). Have a look at
``sample/*`` for details on how to consume this library.

This project supports two kinds of build systems: cmake and make.

Cmake Build
-----------

Cmake is the recommented way for building libosdp.

.. code:: sh

    cmake -B build
    cmake --build build --parallel

Optionally, you could install libosdp using,

.. code:: sh

    cmake --build build --target install

Make Build
----------

The make build is provided as a reference for people who wish to write their
own Makefile to be chained in some other build system. Please see `configure.sh
--help` for different configuration options.

.. code:: sh

    ./configure.sh
    make

Build html docs
---------------

HTML docs of LibOSDP depends on python3, pip3, doxygen, sphinx, and breathe.

.. code:: sh

    pip3 install -r doc/requirements.txt
    cmake -B build
    cmake --build build --target html_docs --parallel


Running the test suite
----------------------

If you are adding features to LibOSDP, you might want to kick off the test
suite to ensure that you changes are valid and has not regressed any other
features. To do this run,

.. code:: sh

    ./scripts/run_tests.sh

Compile-time configuration options
----------------------------------

LibOSDP can can be configured to enable/disable certain featured by passing the
``-DCONFIG_XXX=ON/OFF`` flag to cmake. Following table lists all such config
switches. For instance, if you want to also build static library, you can pass
the flag ``-DCONFIG_OSDP_BUILD_STATIC=ON`` to cmake.

+---------------------+-------------------------------+-----------+-------------------------------------------+
| Configure.sh flag   | CMake OPTION                  | Default   | Description                               |
+=====================+===============================+===========+===========================================+
| --packet-trace      | CONFIG_OSDP_PACKET_TRACE      | OFF       | Enable raw packet trace for diagnostics   |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| --data-trace        | CONFIG_OSDP_DATA_TRACE        | OFF       | Enable command/reply data buffer tracing  |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| --skip-mark         | CONFIG_OSDP_SKIP_MARK_BYTE    | OFF       | Don't send the leading mark byte (0xFF)   |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| --no-colours        | CONFIG_DISABLE_PRETTY_LOGGING | OFF       | Don't colourize log outputs               |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| --static-pd         | CONFIG_OSDP_STATIC_PD         | OFF       | Setup PD single statically                |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| --lib-only          | CONFIG_OSDP_LIB_ONLY          | OFF       | Only build the library                    |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| N/A                 | CONFIG_BUILD_SANITIZER        | ON        | Enable different sanitizers during build  |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| N/A                 | CONFIG_BUILD_SHARED           | ON        | Build shared library                      |
+---------------------+-------------------------------+-----------+-------------------------------------------+
| N/A                 | CONFIG_BUILD_STATIC           | ON        | Build static library                      |
+---------------------+-------------------------------+-----------+-------------------------------------------+

Add LibOSDP to your cmake project
---------------------------------

Cmake find_package
^^^^^^^^^^^^^^^^^^

Build and install LibOSDP to some standard location such ``/usr/local/`` and
then use cmake's popular find_package() method.

Cmake external project
^^^^^^^^^^^^^^^^^^^^^^

Start by adding the following to your CMakeLists.txt

.. code:: cmake

    include(ExternalProject)
    ExternalProject_Add(ext_libosdp
        GIT_REPOSITORY    https://github.com/cbsiddharth/libosdp.git
        GIT_TAG           v3.0.2 # update this to the latest version
        SOURCE_DIR        ${CMAKE_BINARY_DIR}/libosdp/src
        BINARY_DIR        ${CMAKE_BINARY_DIR}/libosdp/build
        CONFIGURE_COMMAND cmake ${CMAKE_BINARY_DIR}/libosdp/src
        BUILD_COMMAND     make
        INSTALL_COMMAND   make install DESTDIR=${CMAKE_BINARY_DIR}/libosdp/install
    )
    include_directories("${CMAKE_BINARY_DIR}/libosdp/install/usr/local/include")
    link_directories("${CMAKE_BINARY_DIR}/libosdp/install/usr/local/lib")

Next, you must add ``ext_libosdp`` as a dependency to your target. That's
it! now you can link your application to osdp library. Following example shows
how you can do this.

.. code:: cmake

    set(OSDP_APP osdp-app)
    list(APPEND OSDP_APP_SRC
        "src/main.c"
        "src/more_source_files.c"
        ...
    )
    add_executable(${OSDP_APP} ${OSDP_APP_SRC})
    add_dependencies(${OSDP_APP} ext_libosdp)
    target_link_libraries(${OSDP_APP} osdp)

Using pkg-config
^^^^^^^^^^^^^^^^

If you are familiar with pkg-config based dependency resolution methods, LibOSDP
provides a libosdp.pc file which is installed along with the library.

Using vcpkg
^^^^^^^^^^^

vcpkg is a free and open-source C/C++ package manager maintained by Microsoft
and the C++ community. A port for LibOSDP has already been merged to vcpkg
upstream -- this means you can consume LibOSDP directly from vcpkg and use all
the generators it supports.

Follow the `getting started document <_DOC>`_ from Microsoft to setup vckpg.
After that, you careate a new application and pull in LibOSDP as a dependency.

.. _DOC: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started

.. code:: shell

    mkdir osdp_app && cd osdp_app
    vcpkg new --application
    vcpkg add port libosdp

After that, you can add your app sources, find the libosdp package and link it
to your target with,

.. code:: cmake

    find_package(LibOSDP CONFIG REQUIRED)
    target_link_libraries(
        main
        PRIVATE $<IF:$<TARGET_EXISTS:libosdp::osdp>,libosdp::osdp,libosdp::osdpstatic>
    )

To build the project, you must set the `CMAKE_TOOLCHAIN_FILE` to the one
provided by vcpkg for the dependencies to be pulled in correctly.

.. code:: shell

    cmake -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -B build .
    cmake --build build

Note: This is the recommended method to consume LibOSDP in Windows platforms.
