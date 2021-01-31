Command structures
==================

LibOSDP exposes the following structures thought ``osdp.h``. This document
attempts to document each of its members. The following structure is used as a
wrapper for all the commands for convenience.

.. code:: c

    struct osdp_cmd {
        enum osdp_cmd_e id;
        union {
            struct osdp_cmd_led    led;
            struct osdp_cmd_buzzer buzzer;
            struct osdp_cmd_text   text;
            struct osdp_cmd_output output;
            struct osdp_cmd_comset comset;
            struct osdp_cmd_keyset keyset;
            struct osdp_cmd_mfg    mfg;
        };
    };

.. doxygenenum:: osdp_cmd_e

.. doxygenstruct:: osdp_cmd
   :members:

Command LED
-----------

.. code:: c

    struct osdp_cmd_led_params {
        uint8_t control_code;
        uint8_t on_count;
        uint8_t off_count;
        uint8_t on_color;
        uint8_t off_color;
        uint16_t timer_count;
    };

    struct osdp_cmd_led {
        uint8_t reader;
        uint8_t number;
        struct osdp_cmd_led_params temporary;
        struct osdp_cmd_led_params permanent;
    };

.. doxygenstruct:: osdp_cmd_led_params
   :members:

.. doxygenstruct:: osdp_cmd_led
   :members:

Command Output
--------------

.. code:: c

    struct osdp_cmd_output {
        uint8_t output_no;
        uint8_t control_code;
        uint16_t timer_count;
    };

.. doxygenstruct:: osdp_cmd_output
   :members:

Command Buzzer
--------------

.. code:: c

    struct osdp_cmd_buzzer {
        uint8_t reader;
        uint8_t control_code;
        uint8_t on_count;
        uint8_t off_count;
        uint8_t rep_count;
    };

.. doxygenstruct:: osdp_cmd_buzzer
   :members:

Command Text
------------

.. code:: c

    struct osdp_cmd_text {
        uint8_t reader;
        uint8_t control_code;
        uint8_t temp_time;
        uint8_t offset_row;
        uint8_t offset_col;
        uint8_t length;
        uint8_t data[32];
    };

.. doxygenstruct:: osdp_cmd_text
   :members:

Command Comset
--------------

.. code:: c

    struct osdp_cmd_comset {
        uint8_t address;
        uint32_t baud_rate;
    };

.. doxygenstruct:: osdp_cmd_comset
   :members:

Command Keyset
--------------

.. code:: c

    struct osdp_cmd_keyset {
        uint8_t type;
        uint8_t length;
        uint8_t data[OSDP_CMD_KEYSET_KEY_MAX_LEN];
    };

.. doxygenstruct:: osdp_cmd_keyset
   :members:
