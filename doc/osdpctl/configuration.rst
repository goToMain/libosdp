Configuration
=============

OSDP devices need many static configuration information to be passed to libOSDP
to setup/maintain a connection. Passing them as command line arguments to
osdpctl isn't really practical, and hence a config file (``ini`` format) was
introduced. There are 2 sections under which configuration keys are housed:

-  GLOBAL - common setting including those that are needed in CP mode.
-  PD - settings used exclusively in PD mode.

Fields indicated with a ``*`` are mandatory in each mode. Some times depending
on the mode, some more keys are required. Such dependencies are listed below each
table.

Read more about `INI file format`_ and you can see some sample configuration
files `here`_.

.. _INI file format: https://en.wikipedia.org/wiki/INI_file
.. _here: https://github.com/cbsiddharth/libosdp/tree/master/osdpctl/config

Global Configuration Keys
-------------------------

The following keys must be under the section ``GLOBAL`` for osdpctl to recognize
and use them.

+------------------+---------------------------------------------------------------+
| Key              | Value                                                         |
+==================+===============================================================+
| mode *           | String: OSDP mode of operation. Can be either "CP" or "PD"    |
+------------------+---------------------------------------------------------------+
| num_pd *         | Integer: Number of PD connected to CP (set to 1 in PD mode)   |
+------------------+---------------------------------------------------------------+
| log_level        | Integer: libOSDP log verbosity level 0 to 7                   |
+------------------+---------------------------------------------------------------+
| conn_topology    | String: CP-PD connection topology. Can be "Chain" or "Star"   |
+------------------+---------------------------------------------------------------+
| pid_file         | Path: file to write process ID. Used with (-f, fork)          |
+------------------+---------------------------------------------------------------+
| log_file         | Path: file to redirect OSDP logs to (will be created)         |
+------------------+---------------------------------------------------------------+

PD Configuration Keys
---------------------

The following keys must be under the section ``PD`` for osdpctl to recognize
and use them.

PD Configuration Keys needed in CP and PD modes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following keys are needed in CP and PD mode.

+--------------------+------------------------------------------------------------+
| Key                | Value                                                      |
+====================+============================================================+
| address *          | Integer: Address of PD as defined in OSDP                  |
+--------------------+------------------------------------------------------------+
| channel_type *     | String: Can be "uart" or "msgq" or "custom" (see below)    |
+--------------------+------------------------------------------------------------+
| channel_speed      | Integer: When type is "uart" this field is the baud rate   |
+--------------------+------------------------------------------------------------+
| channel_device     | String: Path to device. Eg. "/dev/ttyUSB0"                 |
+--------------------+------------------------------------------------------------+
| scbk *             | String: key for SC as hex string (see examples)            |
+--------------------+------------------------------------------------------------+

**Note:**

-  When ``channel_type`` is set to "uart", ``channel_speed`` and ``channel_device``
   are required.

PD Configuration Keys needed only in PD mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following keys are needed only in PD mode.

+--------------------+------------------------------------------------------------+
| Key                | Value                                                      |
+====================+============================================================+
| capabilities       | Complex: See below.                                        |
+--------------------+------------------------------------------------------------+
| key_store          | Path: file to store SCBK (will be created)                 |
+--------------------+------------------------------------------------------------+
| vendor_code *      | Integer: to be reported in response to PD_ID command       |
+--------------------+------------------------------------------------------------+
| model *            | Integer: to be reported in response to PD_ID command       |
+--------------------+------------------------------------------------------------+
| version *          | Integer: to be reported in response to PD_ID command       |
+--------------------+------------------------------------------------------------+
| serial_number *    | Integer: to be reported in response to PD_ID command       |
+--------------------+------------------------------------------------------------+
| firmware_version * | Integer: to be reported in response to PD_ID command       |
+--------------------+------------------------------------------------------------+

Capabilities:
^^^^^^^^^^^^^

PD Capabilities key is expressed as a list of tuples (python-ish):

::

    Capabilities = [ (FC, C, NI), (FC, C, NI), ...  ]

**Fields:**

-  FC: Function Code
-  C: Compliance
-  NI: Number of Items

Refer to the `capabilities document`_ for details on each of the fields and
various function codes.

.. _capabilities document: ../protocol/pd-capabilities.html
