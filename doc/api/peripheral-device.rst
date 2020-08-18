Peripheral Device
=================

The following functions are used when OSDP is to be used in pd mode. The library
returns a single opaque pointer of type ``osdp_t`` where it maintains all it's
internal data. All applications consuming this library must pass this context
pointer all API calls.


Device Setup/Refresh/Teardown
-----------------------------

These functions are used to manage the lifecycle of the device.

osdp_pd_setup
~~~~~~~~~~~~~

.. code:: c

    osdp_t *osdp_pd_setup(osdp_pd_info_t * info, uint8_t *scbk);

This is the fist function your application must invoke to create a OSDP device.
It returns non-NULL on success and NULL on errors.

The ``uint8_t *scbk`` is a pointer to a 16-byte Secure Channel Block key that
the PD will use to initiate Communication with the CP. If this pointer is NULL,
then, the PD would be set to install mode, where the the SCBK-Default key is
used to setup the Secure Channel.

This function must also pass a pointer to ``osdp_pd_info_t`` with the following
attributes pre populated.

.. code:: c

    typedef struct {
        /**
        * Can be one of 9600/38400/115200.
        */
        int baud_rate;

        /**
        * 7 bit PD address. the rest of the bits are ignored. The special address
        * 0x7F is used for broadcast. So there can be 2^7-1 devices on a multi-
        * drop channel.
        */
        int address;

        /**
        * Used to modify the way the context is setup.
        */
        int flags;

        /**
        * Static info that the PD reports to the CP when it received a `CMD_ID`.
        * This is used only in PD mode of operation.
        */
        struct osdp_pd_id id;

        /**
        * This is a pointer to an array of structures containing the PD's
        * capabilities. Use { -1, 0, 0 } to terminate the array. This is used
        * only PD mode of operation.
        */
        struct osdp_pd_cap *cap;

        /**
        * Communication channel ops structure, containing send/recv function
        * pointers.
        */
        struct osdp_channel channel;
    } osdp_pd_info_t;

osdp_pd_refresh
~~~~~~~~~~~~~~~

.. code:: c

    void osdp_pd_refresh(osdp_t *ctx);

This is a periodic refresh method. It must be called at a frequency that seems
reasonable from the applications perspective. Typically, one calls every 50ms
is expected to meet various OSDP timing requirements.

osdp_pd_teardown
~~~~~~~~~~~~~~~~

.. code:: c

    void osdp_pd_teardown(osdp_t *ctx);

This function is used to shutdown communications. All allocated memory is freed
and the ``osdp_t`` context pointer can be discarded after this call.


PD Commands Workflow
--------------------

osdp_pd_get_cmd
~~~~~~~~~~~~~~~

.. code:: c

    int osdp_pd_get_cmd(osdp_t *ctx, struct osdp_cmd *cmd);

This is a periodic poll method. Applications can use this method to pull
commands that are queued to the PD from a CP. Refer to the `command structure`_
document for more information on how to use the ``cmd`` pointer returned by this
function.

.. _command structure: command-structure.html
