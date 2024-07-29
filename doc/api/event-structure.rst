Application Events
==================

LibOSDP exposes the following structures thought ``osdp.h``. This document
attempts to document each of its members. The following structure is used as a
wrapper for all the events for convenience.

.. code:: c

    struct osdp_event {
        enum osdp_event_type type; // Used to select specific event in union
        union {
            struct osdp_event_keypress keypress;
            struct osdp_event_cardread cardread;
            struct osdp_event_mfgrep mfgrep;
            struct osdp_status_report status;
        };
    };

Below are the structure of each of the event structures.


Key press Event
---------------

.. doxygenstruct:: osdp_event_keypress
   :members:

Card read Event
---------------

.. doxygenstruct:: osdp_event_cardread
   :members:

Manufacture specific reply Event
--------------------------------

.. doxygenstruct:: osdp_event_mfgrep
   :members:

Status report request Event
---------------------------

.. doxygenstruct:: osdp_status_report
   :members:

.. doxygenenum:: osdp_status_report_type
