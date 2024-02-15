/*
 * Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/pcap_gen.h>

#include "osdp_common.h"

void osdp_packet_capture_init(struct osdp_pd *pd)
{
	pcap_t *cap;
	char path[128];

	snprintf(path, 128, "/tmp/%spd-%d.pcap",
		 is_pd_mode(pd) ? "" : "cp-", pd->address);
	cap = pcap_create(path, OSDP_PACKET_BUF_SIZE,
			  OSDP_PCAP_LINK_TYPE);
	pd->packet_capture_ctx = (void *)cap;
	if (cap) {
		LOG_WRN("Capturing packets to '%s';"
			" a graceful libosdp teardown is required", path);
	} else {
		LOG_WRN("Packet capture init failed;"
			" check if '%s' is accessible", path);
	}
}

void osdp_packet_capture_finish(struct osdp_pd *pd)
{
	pcap_t *cap = pd->packet_capture_ctx;

	assert(ctx);
	pcap_dump(cap);
}

void osdp_capture_packet(struct osdp_pd *pd, uint8_t *buf, int len)
{
	pcap_t *cap = pd->packet_capture_ctx;

	assert(ctx);
	assert(len <= OSDP_PACKET_BUF_SIZE);
	pcap_add_record(cap, buf, len);
}
