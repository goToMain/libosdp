Communication Channel
=====================

LibOSDP uses a _channel_ communicate with the devices. It is upto the
users to define a channel and pass it to `osdp_{cp,pd}_setup()` method.
A cahnnel is defined as:

.. code:: c

    struct osdp_channel {
        void *data;
        int id;
        read_fn_t recv;
        write_fn_t send;
        flush_fn_t flush;
    };

.. doxygenstruct:: osdp_channel
   :members:

.. doxygentypedef:: osdp_write_fn_t

.. doxygentypedef:: osdp_read_fn_t

.. doxygentypedef:: osdp_flush_fn_t
