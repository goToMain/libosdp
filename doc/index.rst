OSDP - Open Supervised Device Protocol
======================================

.. image:: https://img.shields.io/github/v/release/goToMain/libosdp
   :target: https://github.com/goToMain/libosdp/releases
   :alt: libosdp Version
.. image:: https://img.shields.io/github/license/goToMain/libosdp
   :target: https://github.com/goToMain/libosdp/
   :alt: License
.. image:: https://github.com/goToMain/libosdp/workflows/Build%20CI/badge.svg
   :target: https://github.com/goToMain/libosdp/actions?query=workflow%3A%22Build+CI%22
   :alt: Build status

This is a cross-platform open source implementation of IEC 60839-11-5 Open
Supervised Device Protocol (OSDP). The protocol is intended to improve
interoperability among access control and security products. It supports Secure
Channel (SC) for encrypted and authenticated communication between configured
devices.

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP) over a two-wire RS-485 multi-drop serial
communication channel. Nevertheless, this protocol can be used to transfer
secure data over any stream based physical channel. Have a look at the
`protocol design documents`_ to get a better idea.

This protocol is developed and maintained by `Security Industry Association`_
(SIA).

.. _Security Industry Association: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
.. _protocol design documents: protocol/index.html

Salient Features of LibOSDP
---------------------------

  - Supports secure channel communication (AES-128)
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

Supported Commands and Replies
------------------------------

OSDP has certain command and reply IDs pre-registered. This implementation
of the protocol support only the most common among them. You can see a
`list of commands and replies`_ and their support status in LibOSDP here.

.. _list of commands and replies: protocol/index.html

.. toctree::
   :caption: LibOSDP
   :hidden:
   :maxdepth: 1

   libosdp/build-and-install
   libosdp/cross-compiling
   libosdp/secure-channel
   libosdp/debugging
   libosdp/compatibility

.. toctree::
   :caption: Protocol
   :maxdepth: 2
   :hidden:

   protocol/introduction
   protocol/commands-and-replies
   protocol/packet-structure
   protocol/pd-capabilities
   protocol/faq

.. toctree::
   :caption: API
   :maxdepth: 2
   :hidden:

   api/control-panel
   api/peripheral-device
   api/miscellaneous
   api/command-structure
   api/channel

.. toctree::
   :caption: osdpctl
   :maxdepth: 2
   :hidden:

   osdpctl/introduction
   osdpctl/configuration

.. toctree::
   :caption: Appendix
   :hidden:

   changelog
   license
