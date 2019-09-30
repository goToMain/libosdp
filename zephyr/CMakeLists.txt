# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_OSDP osdp_phy.c)
zephyr_sources_ifdef(CONFIG_OSDP_CONF_MODE_PD osdp_pd.c)
