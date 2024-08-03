# LibOSDP - Open Supervised Device Protocol Library

[![Latest Release][1]][2] [![Build CI][3]][4] [![PyPI Version][16]][12]

This is a cross-platform open source implementation of IEC 60839-11-5 Open
Supervised Device Protocol (OSDP). The protocol is intended to improve
interoperability among access control and security products. It supports Secure
Channel (SC) for encrypted and authenticated communication between configured
devices.

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP) over a two-wire RS-485 multi-drop serial
communication channel. Nevertheless, this protocol can be used to transfer
secure data over any stream based physical channel. Read more about OSDP
[here][21].

This protocol is developed and maintained by [Security Industry Association][20]
(SIA).

## Salient Features of LibOSDP

  - Supports secure channel communication (AES-128) by default and provides a
    custom init-time flag to enforce a higher level of security not mandated by
    the specification
  - Can be used to setup a PD or CP mode of operation
  - Exposes a well defined contract though a single header file
  - Cross-platform; runs on bare-metal, Linux, Mac, and even Windows
  - No run-time memory allocation. All memory is allocated at init-time
  - No external dependencies (for ease of cross compilation)
  - Fully non-blocking, asynchronous design
  - Provides Rust, Python3, and C++ bindings for the C library for faster
    integration into various development phases.
  - Includes dozens of integration and unit tests which are incorporated in CI
    to ensure higher quality of releases.
  - Built-in, sophisticated, debugging infrastructure and tools ([see][14]).

## Usage Overview

A device complying with OSDP can either be a CP or a PD. There can be only one
CP on a bus which can talk to multiple PDs. LibOSDP allows your application to
work either as a CP or a PD so depending on what you want to do you have to do
some things differently.

LibOSDP creates the following constructs which allow interactions between
devices on the OSDP bus. These should not be confused with the protocol
specified terminologies that may use the same names. They are:
  - Channel - Something that allows two OSDP devices to talk to each other
  - Commands - A call for action from a CP to one of its PDs
  - Events - A call for action from a PD to its CP

You start by implementing the `osdp_channel` interface; this allows LibOSDP to
communicate with other osdp devices on the bus. Then you describe the PD you
are
  - talking to on the bus (in case of CP mode of operation) or,
  - going to behave as on the bus (in case of PD mode of operation)
by using the `osdp_pd_info_t` struct.

You can use `osdp_pd_info_t` struct (or an array of it in case of CP) to create
a `osdp_t` context. Then your app needs to call the `osdp_cp/pd_refresh()` as
frequently as possible. To meet the OSDP specified timing requirements, your
app must call this method at least once every 50ms.

After this point, the CP context can,
  - send commands to any one of the PDs (to control LEDs, Buzzers, etc.,)
  - register a callback for events that are sent from a PD

and the PD context can,
  - notify it's controlling CP about an event (card read, key press, etc.,)
  - register a callback for commands issued by the CP

## Language Support

### C/C++ API

LibOSDP core is written in C. It exposes a [minimal set of API][26] to setup
and manage the lifecycle of OSDP devices. See `include/osdp.h` or
`include/osdp.hpp` for more details.

### Rust API

LibOSDP is available via [crates.io][10]. See [rust/README.md][11] for more
info and usage examples.

### Python API

LibOSDP is available as a [python package][12]. See [python/README.md][13] for
more info and usage examples.

## Supported Commands and Replies

OSDP has certain command and reply IDs pre-registered. This implementation of
the protocol support only the most common among them. You can see a list of
commands and replies and their support status in LibOSDP [here][22].

## Dependencies

  * [goToMain/C-Utils][25] (host, submodule)
  * cmake3 (host)
  * python3 (host, optional)
  * python3-pip (host, optional)
  * [doxygen][9] (host, optional; for building the html docs as seen [here][6])
  * [OpenSSL][8] (host and target, optional - recommended)
  * [MbedTLS][7] (host and target, optional)
  * [pytest][5] (host, optional; for running the integrated test suite)
  * python3-venv (host, optional; for running integration test suite)

### For ubuntu

```sh
sudo apt install cmake python3 python3-pip python3-dev python3-venv libssl-dev
```

## Compile LibOSDP

LibOSDP provides a lean-build that only builds the core library and nothing
else. This is useful if you are cross compiling as it doesn't have any other
dependencies but a C compiler. Here is an example of how you can cross compile
LibOSDP to `arm-none-eabi-gcc`.

```
export CROSS_COMPILE=arm-none-eabi-
export CCFLAGS=--specs=nosys.specs
./configure.sh
make
```

To build libosdp and all its components you must have cmake-3.0 (or above) and
a C compiler installed.  This repository produces a `libosdp.so` and
`libosdpstatic.a`; so depending on on your needs you can link these with -losdp
or -losdpstatic, respectively.

Have a look at `examples/*` for a quick lookup on how to consume this library and
structure your application.

You can also read the [API documentation][26] for a comprehensive list of APIs
that are exposed by libosdp.

```sh
git clone https://github.com/goToMain/libosdp --recurse-submodules
cd libosdp
cmake -B build .
cmake --build build --parallel
```

Refer to the [documentation][23] for more information on build and cross
compilation.

### Run the test suite

LibOSDP uses the [pytest][5] python framework to test changes made to ensure
we aren't breaking existing functionalities while adding newer ones. You can
install pytest in your development machine with,

```sh
python3 -m pip install pytest
```

Running the tests locally before creating a pull request is recommended to make
sure that your changes aren't breaking any of the existing functionalities. Here
is how you can run them:

```sh
mkdir build && cd build
cmake ..
make python_install
make check
```

To add new tests for the feature you are working one, see the other tests in
`pytest` directory.

### Build HTML docs

This sections is for those who want to build the HTML documentation for this
project locally. The latest version of the doc can always be found at
[libosdp.sidcha.dev][6].

Install the dependencies (one time) with,

```sh
sudo apt install doxygen
pip3 install -r doc/requirements.txt
```

Build the docs by doing the following:

```sh
mkdir build && cd build
cmake ..
make html_docs # output in ./docs/sphinx/
```

## Contributions, Issues and Bugs

The Github issue tracker doubles up as TODO list for this project. Have a look
at the [open items][31], PRs in those directions are welcome.

If you have a idea, find bugs, or other issues, please [open a new issue][28]
in the github page of this project [https://github.com/goTomain/libosdp][24].

You can read more on this [here](CONTRIBUTING.md).

## License

This software is distributed under the terms of Apache-2.0 license. If you don't
know what that means/implies, you can consider it is as "free as in beer".

OSDP protocol is also open for consumption into any product. There is no need
to,
 - obtain permission from SIA
 - pay royalty to SIA
 - become SIA member

The OSDP specification can be obtained from SIA for a cost. Read more at our
[FAQ page][27].

## Support the development

Since this is no longer a hobby project, it takes time and effort to develop
and maintain this project. If you are a user and are happy with it, consider
supporting the development by donations though my [GitHub sponsors page][15].
Your support will ensure sustained development of LibOSDP.

[1]: https://img.shields.io/github/v/release/GoToMain/libosdp?display_name=tag&logo=github
[2]: https://github.com/goToMain/libosdp/releases/latest
[3]: https://github.com/goTomain/libosdp/workflows/Build%20CI/badge.svg
[4]: https://github.com/goTomain/libosdp/actions?query=workflow%3A%22Build+CI%22
[5]: https://docs.pytest.org/en/latest/
[6]: https://libosdp.sidcha.dev/
[7]: https://github.com/ARMmbed/mbedtls
[8]: https://www.openssl.org/
[9]: https://www.doxygen.nl/index.html
[10]: https://crates.io/crates/libosdp
[11]: https://github.com/goToMain/libosdp-rs/tree/master/libosdp
[12]: https://pypi.org/project/libosdp/
[13]: https://github.com/goToMain/libosdp/tree/master/python
[14]: https://libosdp.sidcha.dev/libosdp/debugging
[15]: https://github.com/sponsors/sidcha
[16]: https://img.shields.io/pypi/v/libosdp?logo=python&link=https%3A%2F%2Fpypi.org%2Fproject%2Flibosdp%2F

[20]: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
[21]: https://libosdp.sidcha.dev/protocol/
[22]: https://libosdp.sidcha.dev/protocol/commands-and-replies.html
[23]: https://libosdp.sidcha.dev/libosdp/build-and-install.html
[24]: https://github.com/goTomain/libosdp
[25]: https://github.com/goTomain/c-utils
[26]: https://libosdp.sidcha.dev/api/
[27]: https://libosdp.sidcha.dev/protocol/faq.html
[28]: https://github.com/goToMain/libosdp/issues/new/choose
[29]: https://github.com/goToMain/libosdp/blob/master/python
[30]: https://github.com/goToMain/libosdp/tree/master/tests/pytest/testlib
[31]: https://github.com/goToMain/libosdp/issues
