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
	int n;
	char path[128];

	n = snprintf(path, sizeof(path), "osdp-trace-%spd-%d-",
		     is_pd_mode(pd) ? "" : "cp-", pd->address);
	n += add_iso8601_utc_datetime(path + n, sizeof(path) - n);
	strcpy(path + n, ".pcap");
	cap = pcap_create(path, OSDP_PACKET_BUF_SIZE,
			  OSDP_PCAP_LINK_TYPE);
	if (cap) {
		LOG_WRN("Tracing: capturing packets to '%s'");
		LOG_WRN("Tracing: a graceful teardown of libosdp ctx is required"
			" for a complete trace file to be produced.", path);
	} else {
		LOG_WRN("Tracing: packet capture init failed;"
			" check if path '%s' is accessible", path);
	}
	pd->packet_capture_ctx = (void *)cap;
}

void osdp_packet_capture_finish(struct osdp_pd *pd)
{
	pcap_t *cap = pd->packet_capture_ctx;

	assert(cap);
	pcap_dump(cap);
}

void osdp_capture_packet(struct osdp_pd *pd, uint8_t *buf, int len)
{
	pcap_t *cap = pd->packet_capture_ctx;

	assert(cap);
	assert(len <= OSDP_PACKET_BUF_SIZE);
	pcap_add_record(cap, buf, len);
}
