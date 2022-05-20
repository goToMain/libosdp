Control Panel
=============

The following functions are used when OSDP is to be used in CP mode.

Device lifecycle management
---------------------------

.. doxygentypedef:: osdp_t

.. doxygenstruct:: osdp_pd_info_t
   :members:

.. doxygenfunction:: osdp_cp_setup2

.. doxygenfunction:: osdp_cp_refresh

.. doxygenfunction:: osdp_cp_teardown

Events
------

.. doxygenstruct:: osdp_event

.. doxygentypedef:: cp_event_callback_t

.. doxygenfunction:: osdp_cp_set_event_callback

Commands
--------

For the CP application, it's connected PDs are referenced by the offset number.
This offset corresponds to the order in which the ``osdp_pd_info_t`` was
populated when passed to ``osdp_cp_setup``.

.. doxygenfunction:: osdp_cp_send_command

Refer to the `command structure`_ document for more information on how to
populate the ``cmd`` structure for these function.

.. _command structure: command-structure.html

Get PD capability
-----------------

.. doxygenstruct:: osdp_pd_cap
   :members:

.. doxygenfunction:: osdp_cp_get_capability

Others
------

.. doxygenfunction:: osdp_cp_get_pd_id

.. doxygenfunction:: osdp_cp_modify_flag

.. doxygenfunction:: osdp_cp_get_io_status