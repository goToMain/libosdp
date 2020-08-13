Control Panel API
=================

The following functions are used when OSDP is to be used in CP mode.

Device Setup/Refresh/Teardown
-----------------------------

.. code:: c

   osdp_t *osdp_cp_setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key);

This is the fist function your application must invoke to create a OSDP device.
It returns non-NULL on success and NULL on errors.

The ``uint8_t *master_key`` is a pointer to a 16-byte Master key that the CP
will use to initiate Communication with all the PDs. This is mandatory argument.

In Addition to the master key, the application must also pass pointer to an
array of type ``osdp_pd_info_t`` of length ``num_pd``. The order in which the
PDs appear in this pointer is the order in which the PD will be referred to in
all further interactions.

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

.. code:: c

    void osdp_cp_refresh(osdp_t *ctx);

This is a periodic refresh method. It must be called at a frequency that seems
reasonable from the applications perspective. Typically, one calls every 50ms
is expected to meet various OSDP timing requirements.

.. code:: c

   void osdp_cp_teardown(osdp_t *ctx);

This function is used to shutdown communications. All allocated memory is freed
and the ``osdp_t`` context pointer can be discarded after this call.

Key press and Card read notifiers
---------------------------------

.. code:: c

    int osdp_cp_set_callback_key_press(osdp_t *ctx, int (*cb) (int address, uint8_t key));
    int osdp_cp_set_callback_card_read(osdp_t *ctx, int (*cb) (int address, int format, uint8_t * data, int len));

Both these functions are used to register a callback function for key press and
card read event. libosdp will invoke these callback methods when the
corresponding event occurs.

CP Commands Workflow
--------------------

For the CP application it's connected PDs are referenced by the offset number.
this offset corresponds to the order in which the ``osdp_pd_info_t`` was
populated when passed to ``osdp_cp_setup``.

.. code:: c

    int osdp_cp_send_cmd_led(osdp_t *ctx, int pd, struct osdp_cmd_led *p);
    int osdp_cp_send_cmd_buzzer(osdp_t *ctx, int pd, struct osdp_cmd_buzzer *p);
    int osdp_cp_send_cmd_output(osdp_t *ctx, int pd, struct osdp_cmd_output *p);
    int osdp_cp_send_cmd_text(osdp_t *ctx, int pd, struct osdp_cmd_text *p);
    int osdp_cp_send_cmd_comset(osdp_t *ctx, int pd, struct osdp_cmd_comset *p);
    int osdp_cp_send_cmd_keyset(osdp_t *ctx, struct osdp_cmd_keyset *p);

All of these methods are used to send a specific command to the PD with offset
number ``int pd``. A return value of 0 indicates success.

Refer to the `command structure <command-structure>`_ document for more
information on how to populate the ``cmd`` structure for these function.
