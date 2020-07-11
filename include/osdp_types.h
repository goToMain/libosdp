/*
 * Copyright (c) 2020 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_TYPES_H_
#define _OSDP_TYPES_H_

/* Imported from osdp.h -- Don't include directly */

#include <stdint.h>

enum osdp_card_formats_e {
	OSDP_CARD_FMT_RAW_UNSPECIFIED,
	OSDP_CARD_FMT_RAW_WIEGAND,
	OSDP_CARD_FMT_ASCII,
	OSDP_CARD_FMT_SENTINEL
};

/* struct pd_cap::function_code */
enum osdp_pd_cap_function_code_e {
	CAP_UNUSED,
	CAP_CONTACT_STATUS_MONITORING,
	CAP_OUTPUT_CONTROL,
	CAP_CARD_DATA_FORMAT,
	CAP_READER_LED_CONTROL,
	CAP_READER_AUDIBLE_OUTPUT,
	CAP_READER_TEXT_OUTPUT,
	CAP_TIME_KEEPING,
	CAP_CHECK_CHARACTER_SUPPORT,
	CAP_COMMUNICATION_SECURITY,
	CAP_RECEIVE_BUFFERSIZE,
	CAP_LARGEST_COMBINED_MESSAGE_SIZE,
	CAP_SMART_CARD_SUPPORT,
	CAP_READERS,
	CAP_BIOMETRICS,
	CAP_SENTINEL
};

struct pd_cap {
	/**
	 * Each PD capability has a 3 byte representation:
	 *   function_code:    One of enum osdp_pd_cap_function_code_e.
	 *   compliance_level: A function_code dependent number that indicates
	 *                     what the PD can do with this capability.
	 *   num_items:        Number of such capability entities in PD.
	 */
	uint8_t function_code;
	uint8_t compliance_level;
	uint8_t num_items;
};

struct pd_id {
	int version;  /* 3-bytes IEEE assigned OUI  */
	int model;    /* 1-byte Manufacturer's model number */
	uint32_t vendor_code;   /* 1-Byte Manufacturer's version number */
	uint32_t serial_number; /* 4-byte serial number for the PD */
	uint32_t firmware_version; /* 3-byte version (major, minor, build) */
};

struct osdp_channel {
	/**
	 * pointer to a block of memory that will be passed to the send/receive
	 * method. This is optional and can be left empty.
	 */
        void *data;

	/**
	 * recv: function pointer; Copies received bytes into buffer
	 * @data - for use by underlying layers. channel_s::data is passed
	 * @buf  - byte array copy incoming data
	 * @len  - sizeof `buf`. Can copy utmost `len` number of bytes into `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes copied on to `bug`. Must be <= `len`
	 */
	int (*recv)(void *data, uint8_t *buf, int maxlen);

	/**
	 * send: function pointer; Sends byte array into some channel
	 * @data - for use by underlying layers. channel_s::data is passed
	 * @buf  - byte array to be sent
	 * @len  - number of bytes in `buf`
	 *
	 * Returns:
	 *  +ve: number of bytes sent. must be <= `len`
	 */
	int (*send)(void *data, uint8_t *buf, int len);

	/**
	 * flush: function pointer; drop all bytes in queue to be read
	 * @data - for use by underlying layers. channel_s::data is passed
	 */
	void (*flush)(void *data);
};

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
	struct pd_id id;

	/**
	 * This is a pointer to an array of structures containing the PD's
	 * capabilities. Use macro `OSDP_PD_CAP_SENTINEL` to terminate the array.
	 * This is used only PD mode of operation.
	 */
	struct pd_cap *cap;

	/**
	 * Communication channel ops structure, containing send/recv function
	 * pointers.
	 */
	struct osdp_channel channel;
} osdp_pd_info_t;

struct osdp_cp_notifiers {
	int (*keypress) (int address, uint8_t key);
	int (*cardread) (int address, int format, uint8_t * data, int len);
};

/**
 * The keep the OSDP internal data strucutres from polluting the exposed headers,
 * they are typedefed to void before sending them to the upper layers. This
 * level of abstaction looked reasonable as _technically_ no one should attempt
 * to modify it outside fo the LibOSDP and their definition may change at any
 * time.
 */
typedef void osdp_t;

#endif /* _OSDP_TYPES_H_ */
