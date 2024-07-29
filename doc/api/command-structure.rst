Application Commands
====================

LibOSDP exposes the following structures thought ``osdp.h``. This document
attempts to document each of its members. The following structure is used as a
wrapper for all the commands for convenience.

.. code:: c

    struct osdp_cmd {
        enum osdp_cmd_e id; // Command ID. Used to select specific commands in union
        union {
            struct osdp_cmd_led led;
            struct osdp_cmd_buzzer buzzer;
            struct osdp_cmd_text text;
            struct osdp_cmd_output output;
            struct osdp_cmd_comset comset;
            struct osdp_cmd_keyset keyset;
            struct osdp_cmd_mfg mfg;
	    struct osdp_cmd_file_tx file_tx;
	    struct osdp_status_report status;
        };
    };

Below are the structure of each of the command structures.

LED command
-----------

.. doxygenstruct:: osdp_cmd_led_params
   :members:

.. doxygenstruct:: osdp_cmd_led
   :members:

Buzzer command
--------------

.. doxygenstruct:: osdp_cmd_buzzer
   :members:

Text command
------------

.. doxygenstruct:: osdp_cmd_text
   :members:

Output command
--------------

.. doxygenstruct:: osdp_cmd_output
   :members:

Comset command
--------------

.. doxygenstruct:: osdp_cmd_comset
   :members:

Keyset command
--------------

.. doxygenstruct:: osdp_cmd_keyset
   :members:

Manufacture specific command
----------------------------

.. doxygenstruct:: osdp_cmd_mfg
   :members:

File transfer command
---------------------

.. doxygenstruct:: osdp_cmd_file_tx
   :members:

Status report command
---------------------

.. doxygenstruct:: osdp_status_report
   :members:

.. doxygenenum:: osdp_status_report_type
