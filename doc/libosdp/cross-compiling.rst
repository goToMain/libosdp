Cross Compiling
---------------

LibOSDP can be compiled with your cross compiler by passing a toolchain file to
cmake. This can be done by invoking cmake with the command line argument
``-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain-file.cmake``.

If your toolchain is installed in ``/opt/toolchain/armv8l-linux-gnueabihf/`` and
the sysroot is present in ``/opt/toolchain/armv8l-linux-gnueabihf/sysroot``, the
``toolchain-file.cmake`` file should look like this:

.. code:: cmake

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)

    # specify the cross compiler and sysroot
    set(TOOLCHAIN_INST_PATH /opt/toolchain/armv8l-linux-gnueabihf)
    set(CMAKE_C_COMPILER    ${TOOLCHAIN_INST_PATH}/bin/armv8l-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER  ${TOOLCHAIN_INST_PATH}/bin/armv8l-linux-gnueabihf-g++)
    set(CMAKE_SYSROOT       ${TOOLCHAIN_INST_PATH}/sysroot)

    # don't search for programs in the build host directories
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

    # search for libraries and headers in the target directories only
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

For convenience, the ``toolchain-file.cmake`` file can be placed in a common path
(probably where the toolchain is installed) and referenced from our build directory.

.. code:: sh

    mkdir build && cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain/armv8l-linux-gnueabihf/toolchain-file.cmake ..
    make
