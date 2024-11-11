PD Info
=======

``osdp_pd_info_t`` is a user provided structure which describes how the PD has to
be setup. In CP mode, the app described the PDs it would like to communicate 
with using an array of ``osdp_pd_info_t`` structures. 

.. doxygenstruct:: osdp_pd_info_t
   :members:

Setup Flags
-----------

OSDP setup in CP or PD mode can be infulenced by the following flags (set in
``osdp_pd_info_t::flags``). Some of them are effective only in CP or PD mode; see
individual flag documentation below.

.. doxygendefine:: OSDP_FLAG_ENFORCE_SECURE

.. doxygendefine:: OSDP_FLAG_INSTALL_MODE

.. doxygendefine:: OSDP_FLAG_IGN_UNSOLICITED

.. doxygendefine:: OSDP_FLAG_ENABLE_NOTIFICATION

.. doxygendefine:: OSDP_FLAG_CAPTURE_PACKETS

.. doxygendefine:: OSDP_FLAG_ALLOW_EMPTY_ENCRYPTED_DATA_BLOCK
