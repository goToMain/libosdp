# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_OSDP
	osdp_zephyr.c
	src/osdp_phy.c
	src/osdp_common.c
)

zephyr_sources_ifdef(CONFIG_OSDP_CONF_MODE_PD
	src/osdp_pd.c
)

zephyr_sources_ifdef(CONFIG_OSDP_CONF_MODE_CP
	src/osdp_cp.c
)
