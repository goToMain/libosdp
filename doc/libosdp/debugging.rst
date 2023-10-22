Debugging
=========

LibOSDP has a lot of debugging/diagnostics built into it to help narrow down the
issue without having to exchange a bunch of emails/comments with each user who
encounters an issue. This save developer time at both ends. That said, the
default logging methods are very sane which makes this unnecessary for every
issue. So don't trouble yourself to generate such logs proactively.

A note on the log file
----------------------

It is preferable to attach the full logs (from osdp_cp_setup() to point of
failure). If you feel that you know what the issue is or that the entire log
wont help, please feel free to snip them as you see fit, but please try to
retain the very first few log lines as it has some info on which version of
LibOSDP you are running and you PD connection topology.

When creating an issue in GitHub, it is not very elegant to post the entire log
file in the issue description so it is better to attached the log file to the
issue or post it in some log sharing websites such as https://pastebin.com/ and
include the link in the issue.

Log Level
---------

LibOSDP supports different logging levels with ``LOG_DEBUG`` being the most
verbose mode. When asking for help, please set the log level to ``LOG_DEBUG``.

This can be done by calling osdp_logger_init3() BEFORE calling osdp_cp/pd_setup()
as,

.. code:: c

    osdp_logger_init3("osdp::cp", LOG_DEBUG, uart_puts);

Packet Trace Builds
-------------------

This is the most verbose form of debugging where all bytes on the wire are
logged as LibOSDP saw them. This can come in handy when trying to debug low
level issues.

To enable packet trace builds, follow these steps:

.. code:: sh

    mkdir build-pt && cd build-pt
    cmake -DCONFIG_OSDP_PACKET_TRACE=on ..
    make


Data Trace Builds
-----------------

When secure channel is working fine and you are encountering a command level
failure, it can be helpful to see the decrypted messages instead of the junk
that would get dumped when secure channel is enabled.

To enable data trace builds, follow these steps:

.. code:: sh

    mkdir build-dt && cd build-dt
    cmake -DCONFIG_OSDP_DATA_TRACE=on ..
    make

Note: It is seldom useful to run on both packet trace AND data trace (in fact it
makes it harder to locate relevant information) so please never do it.