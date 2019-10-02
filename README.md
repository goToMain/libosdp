![OSDP - Open Supervised Device Protocol][4]

[![Build Status][1]][2]

This is an open source implementation of Open Supervised Device Protocol (OSDP)
developed by [Security Industry Association (SIA)][3]. The protocol is intended
to improve interoperability among access control and security products. OSDP
is currently in-process to become a standard recognized by the American National
Standards Institute (ANSI).

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP). The OSDP specification describes the
protocol implementation over a two-wire RS-485 multi-drop serial communication
channel. Nevertheless, this protocol can be used to transfer secure data over
any physical channel. Have a look at our [protocol design documents][5] to get
a better idea.

## Salient Features of LibOSDP

- Supports secure channel communication (AES-128).
- LibOSDP can be used to setup a PD or CP.
- Exposed a well defined contract though `include/osdp.h`.
- No runtime memory allocation. All memory is allocated at init-time.
- Well designed source code architecure.

## Supported Commands and Replies

OSDP has certain command and reply IDs pre-registered. This implementation of
the protocol support only the most common among them. You can see [a list of
commands and replies and their support status in LibOSDP here][6].

## Compile LibOSDP

Build required you to have `cmake3` or above and a C compiler installed. This
repository builds a `libosdp.a` that you can link with your application. Have a
look at `sample/*` for details on how to consume this library.

```sh
mkdir build
cd build
cmake ..
make
```

This repository is a work in progress; read the `TODO` file for list of pending
tasks. Patches in those areas are welcome; open an issue if you find a bug.

[1]: https://travis-ci.org/cbsiddharth/libosdp.svg?branch=master
[2]: https://travis-ci.org/cbsiddharth/libosdp
[3]: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
[4]: doc/osdp-logo.png
[5]: doc/osdp-design.md
[6]: doc/osdp-commands-and-replies.md
