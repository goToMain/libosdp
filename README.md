# libosdp

[![Build Status][1]][2]

This is an open source implementation of Open Supervised Device Protocol (OSDP)
developed by [Security Industry Association (SIA)][3]. The protocol is intended
to improve interoperability among access control and security products. OSDP
is currently in-process to become a standard recognized by the American National
Standards Institute (ANSI).

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP). This document specifies the protocol
implementation over a two-wire RS-485 multi-drop serial communication channel.
In theory, this protocol can be used to transfer secure data over any physical


## Compile libosdp

```sh
mkdir build
cd build
cmake ..
make
```

[1]: https://travis-ci.org/cbsiddharth/libosdp.svg?branch=master
[2]: https://travis-ci.org/cbsiddharth/libosdp
[3]: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
