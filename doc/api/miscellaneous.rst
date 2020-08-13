Miscellaneous
=============

Debugging and Diagnostics
-------------------------

osdp_logger_init
~~~~~~~~~~~~~~~~

.. code:: c

    void osdp_logger_init(int log_level, int (*log_fn)(const char *fmt, ...));

This function is used to the global log level and an optional pointer to printf
like function ``log_fn`` that will be called when a log message must be printed.
This is useful if your logs are redirected elsewhere (such as to a UART device).

osdp_get_version
~~~~~~~~~~~~~~~~

.. code:: c

    const char *osdp_get_version();

Get a string representation of libosdp version. This can be used in logs and
can come in handy when debugging.

osdp_get_source_info
~~~~~~~~~~~~~~~~~~~~

.. code:: c

    const char *osdp_get_source_info();

Get a string that tried to identify the origin of the source tree from which the
libosdp was built. It has information such as git branch, tag and other useful
information when debugging issues.

Status
-------

osdp_get_status_mask
~~~~~~~~~~~~~~~~~~~~

.. code:: c

    uint32_t osdp_get_status_mask(osdp_t *ctx);

This function is used to get a mask of all PDs online at the moment. The order
is the same as that specified in the ``osdp_pd_info_t`` structure passed to
setup the OSDP device.

osdp_get_sc_status_mask
~~~~~~~~~~~~~~~~~~~~~~~

This function is used to get a mask of all PDs that have an active secure
channel at the moment. The order is the same as that specified in the
``osdp_pd_info_t`` structure passed to setup the OSDP device.

.. code:: c

    uint32_t osdp_get_sc_status_mask(osdp_t *ctx);
