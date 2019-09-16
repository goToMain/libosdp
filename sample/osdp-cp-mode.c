/**
 *  SPDX-License-Identifier: MIT
 *
 *  Author: Siddharth Chandrasekaran
 *    Date: Sat Sep 14 14:02:33 IST 2019
 */

#include <osdp.h>

enum osdp_pd_e {
    OSDP_PD_1,
    OSDP_PD_2,
    OSDP_PD_SENTINEL,
};

int sample_send_func(uint8_t *buf, int len)
{
}

int sample_recv_func(uint8_t *buf, int len)
{
}

int main()
{
    osdp_pd_info_t info[OSDP_PD_SENTINEL] = {
        [OSDP_PD_1] = {
            .address = 101,
            .baud_rate = 9600,
            .init_flags = 0,
            .send_func = sample_send_func,
            .recv_func = sample_recv_func
	    },
        [OSDP_PD_2] = {
            .address = 102,
            .baud_rate = 9600,
            .init_flags = 0,
            .send_func = sample_send_func,
            .recv_func = sample_recv_func
        },
    };

    osdp_cp_t *ctx = osdp_cp_setup(OSDP_PD_SENTINEL, &info[0]);

    return 0;
}
