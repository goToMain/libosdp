OSDP - Open Supervised Device Protocol
======================================

.. image:: https://img.shields.io/github/v/release/goToMain/libosdp
   :target: https://github.com/goToMain/libosdp/releases
   :alt: libosdp Version
.. image:: https://img.shields.io/github/license/goToMain/libosdp
   :target: https://github.com/goToMain/libosdp/
   :alt: License
.. image:: https://api.codacy.com/project/badge/Grade/7b6f389d4fbf46a692b64d3e82452af9
   :target: https://app.codacy.com/manual/siddharth_6/libosdp?utm_source=github.com&utm_medium=referral&utm_content=cbsiddharth/libosdp&utm_campaign=Badge_Grade_Dashboard
   :alt: Code Coverage
.. image:: https://github.com/goToMain/libosdp/workflows/Build%20CI/badge.svg
   :target: https://github.com/goToMain/libosdp/actions?query=workflow%3A%22Build+CI%22
   :alt: Build status

This is an open source implementation of Open Supervised Device Protocol
(OSDP) developed by `Security Industry Association`_  (SIA).
The protocol is intended to improve interoperability among access control and security
products. OSDP is currently in-process to become a standard recognized by the
American National Standards Institute (ANSI).

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP). The OSDP specification describes the protocol
implementation over a two-wire RS-485 multi-drop serial communication channel.
Nevertheless, this protocol can be used to transfer secure data over any physical
channel. Have a look at the `protocol design documents`_ to get a better idea.

.. _Security Industry Association: https://www.securityindustry.org/industry-standards/open-supervised-device-protocol/
.. _protocol design documents: protocol/index.html

Salient Features of LibOSDP
---------------------------

-  Supports secure channel communication (AES-128).
-  Can be used to setup a PD or CP mode of operation.
-  Exposes a well defined contract though ``include/osdp.h``.
-  No run-time memory allocation. All memory is allocated at init-time.
-  Well designed source code architecture.

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
