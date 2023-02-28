Peripheral Device
=================

The following functions are used when OSDP is to be used in pd mode. The library
returns a single opaque pointer of type ``osdp_t`` where it maintains all it's
internal data. All applications consuming this library must pass this context
pointer all API calls.


Device lifecycle management
---------------------------

.. doxygentypedef:: osdp_t

.. doxygenstruct:: osdp_pd_info_t
   :members:

.. doxygenfunction:: osdp_pd_setup

.. doxygenfunction:: osdp_pd_refresh

.. doxygenfunction:: osdp_pd_teardown


PD Capabilities
---------------

.. doxygenstruct:: osdp_pd_cap
   :members:

.. doxygenfunction:: osdp_pd_set_capabilities

Commands
--------

.. doxygentypedef:: pd_command_callback_t

.. doxygenfunction:: osdp_pd_set_command_callback

Refer to the `command structure`_ document for more information on how to
populate the ``cmd`` structure for these function.

.. _command structure: command-structure.html

Events
------

.. doxygenstruct:: osdp_event
   :members:

.. doxygenfunction:: osdp_pd_notify_event

.. doxygenfunction:: osdp_pd_flush_events
