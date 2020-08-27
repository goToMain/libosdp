# LibOSDP - Open Supervised Device Protocol Library

[![Version][1]][2] [![Build CI][3]][4] [![Travis CI][5]][6] [![Codacy Badge][7]][8]

This is an open source implementation of Open Supervised Device Protocol (OSDP)
developed by [Security Industry Association (SIA)][20]. The protocol is intended
to improve interoperability among access control and security products. OSDP
is currently in-process to become a standard recognized by the American National
Standards Institute (ANSI).

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP). The OSDP specification describes the
protocol implementation over a two-wire RS-485 multi-drop serial communication
channel. Nevertheless, this protocol can be used to transfer secure data over
any physical channel. Have a look at our [protocol design documents][21] to get
a better idea.

## Salient Features of LibOSDP

  - Supports secure channel communication (AES-128).
  - Can be used to setup a PD or CP mode of operation.
  - Exposes a well defined contract though `include/osdp.h`.
  - No run-time memory allocation. All memory is allocated at init-time.
  - Well designed source code architecture

## API

LibOSDP exposes a [minimal set of API][26] to setup and manage the lifecycle of
OSDP devices.

## Supported Commands and Replies

OSDP has certain command and reply IDs pre-registered. This implementation of
the protocol support only the most common among them. You can see a list of
commands and replies and their support status in LibOSDP [here][22].

## Dependencies

  * cmake3 (host)
  * python (doc/requirements.txt) (host, optional)
  * [goToMain/C-Utils][25] (host, submodule)

## Compile LibOSDP

To build libosdp you must have cmake-3.0 (or above) and a C compiler installed.
This repository produces a `libosdpstatic.a` and `libosdp.so`. You can link
these with your application as needed (-losdp or -losdpstatic). Have a look at
`sample/* for a quick lookup on how to consume this library and structure your
application.

You can als read the [API documentation][26] for a comprehensive list of APIs
that are exposed by libosdp.

```sh
git clone https://github.com/goToMain/libosdp --recurse-submodules
cd libosdp
mkdir build && cd build
cmake ..
make
make check

# optionally
make DESTDIR=/your/install/path install
```

Refer to [this document][23] for more information on build and cross
compilation.

## Contributions and bugs

This repository is a work in progress; read the `TODO` file for list of pending
tasks. Patches in those areas are welcome; open an issue in the github page of
this project [https://github.com/goTomain/libosdp][24] if you face any issues.

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

[1]: https://img.shields.io/github/v/release/goToMain/libosdp
[2]: https://github.com/goToMain/libosdp/releases
[3]: https://github.com/goTomain/libosdp/workflows/Build%20CI/badge.svg
[4]: https://github.com/goTomain/libosdp/actions?query=workflow%3A%22Build+CI%22
[5]: https://travis-ci.org/goTomain/libosdp.svg?branch=master
[6]: https://travis-ci.org/goTomain/libosdp
[7]: https://api.codacy.com/project/badge/Grade/7b6f389d4fbf46a692b64d3e82452af9
[8]: https://app.codacy.com/manual/siddharth_6/libosdp?utm_source=github.com&utm_medium=referral&utm_content=cbsiddharth/libosdp&utm_campaign=Badge_Grade_Dashboard

[20]: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
[21]: https://libosdp.gotomain.io/protocol/
[22]: https://libosdp.gotomain.io/protocol/commands-and-replies.html
[23]: https://libosdp.gotomain.io/libosdp/build-and-install.html
[24]: https://github.com/goTomain/libosdp
[25]: https://github.com/goTomain/c-utils
[26]: https://libosdp.gotomain.io/api/
[27]: https://libosdp.gotomain.io/protocol/faq.html
