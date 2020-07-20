/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_CONFIG_H_
#define _OSDP_CONFIG_H_

/**
 * @brief The following macros are defined defined from the variabled in cmake
 * files. All  are replaced by the value of XXX as resolved by cmake.
 */
#define PROJECT_VERSION                "1.1.0"
#define PROJECT_NAME                   "libosdp"
#define GIT_BRANCH                     "master"
#define GIT_REV                        "bb7748a"
#define GIT_TAG                        ""
#define GIT_DIFF                       "+"

/**
 * @brief CONFIG_XXX macros are defined by the below cmakedefine statement by
 * cmake during project configuration. These options can be enabled by passing
 * -DCONFIG_XXX=on to cmake.
 */
#define CONFIG_OSDP_PACKET_TRACE

/**
 * @brief Other OSDP constants
 */
#define OSDP_PD_ERR_RETRY_SEC                   (300 * 1000)
#define OSDP_PD_SC_RETRY_SEC                    (600 * 1000)
#define OSDP_PD_POLL_TIMEOUT_MS                 (50)
#define OSDP_RESP_TOUT_MS                       (200)
#define OSDP_CMD_RETRY_WAIT_MS                  (500)
#define OSDP_PACKET_BUF_SIZE                    (512)
#define OSDP_PD_CMD_QUEUE_SIZE                  (128)
#define OSDP_CP_CMD_POOL_SIZE                   (32)

#endif /* _OSDP_CONFIG_H_ */
