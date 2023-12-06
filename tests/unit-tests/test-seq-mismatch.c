#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <osdp.h>
#include <unistd.h>
#include <pthread.h>

#include "test.h"
#include "osdp_common.h"

struct RxBuffer{
	uint8_t buffer[256];
	int buffer_len;
};

struct ThreadParams {
	osdp_t *pd_ctx;
	osdp_t *cp_ctx;
	int keep_alive;
};

pthread_mutex_t params_mutex = PTHREAD_COND_INITIALIZER;
int cmd_count = 0;
static struct RxBuffer cp_rx_buffer;
static struct RxBuffer pd_rx_buffer;

static int test_cp_seq_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	memcpy(pd_rx_buffer.buffer + pd_rx_buffer.buffer_len, buf, len);
	pd_rx_buffer.buffer_len += len;

	return len;
}

static int test_cp_seq_recv(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

	if (cp_rx_buffer.buffer_len == 0)
        	return 0;

    	int maxBytesToCopy = len < cp_rx_buffer.buffer_len ? len : cp_rx_buffer.buffer_len;
    	memcpy(buf, cp_rx_buffer.buffer, maxBytesToCopy);

 	if (maxBytesToCopy != cp_rx_buffer.buffer_len)
        	printf("wrong bytes to copy\n");

	// Clear the buffer
	cp_rx_buffer.buffer_len = 0;

    	return maxBytesToCopy;
}

static int test_pd_seq_send(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);
	memcpy(cp_rx_buffer.buffer + cp_rx_buffer.buffer_len, buf, len);
	cp_rx_buffer.buffer_len += len;

	return len;
}

static int test_pd_seq_recv(void *data, uint8_t *buf, int len)
{
	ARG_UNUSED(data);

 	if (pd_rx_buffer.buffer_len == 0 )
		return 0;

	int maxBytesToCopy = len < pd_rx_buffer.buffer_len ? len : pd_rx_buffer.buffer_len;
	memcpy(buf, pd_rx_buffer.buffer, maxBytesToCopy );
    	if (maxBytesToCopy != pd_rx_buffer.buffer_len) 
        	printf("wrong bytes to copy\n");

    	// Clear the buffer
    	pd_rx_buffer.buffer_len = 0;

    	return maxBytesToCopy;
}

int test_pd_command_handler(void *arg, struct osdp_cmd *cmd)
{
	 printf("PD: CMD: %d\n", cmd->id);
	fflush(stdout);

 	if (cmd->id == OSDP_CMD_MFG) {
		cmd_count++;
		if(cmd_count == 2)
			usleep(250 * 1000);
    	}

	return 0;
}

void pd_thread(void* data)
{
 	osdp_t *pd_ctx;
 	pthread_mutex_lock(&params_mutex);
 	struct ThreadParams* params = (struct ThreadParams*) data;
	pd_ctx = params->pd_ctx;
	pthread_mutex_unlock(&params_mutex);

	while (params->keep_alive) {
        	// your application code.
        	osdp_pd_refresh(pd_ctx);

		usleep(20 * 1000);
    	}
}


int test_pd_seq_setup(struct ThreadParams *params)
{

	/* mock application data */
	struct osdp_pd_cap caps[3];
	caps[0].function_code = OSDP_PD_CAP_READER_LED_CONTROL;
	caps[0].compliance_level = 1;
	caps[0].num_items = 1;
	caps[1].function_code = OSDP_PD_CAP_READER_AUDIBLE_OUTPUT;
	caps[1].compliance_level = 1;
 	caps[1].num_items = 1;
 	caps[2].function_code = -1;
	caps[2].compliance_level = 0;
	caps[2].num_items = 0;

	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 115200,
		.flags = 0,
		.id.version = 1,
		.id.model = 153,
		.id.vendor_code = 31337,
		.id.serial_number = 0x01020304,
 		.id.firmware_version = 0x0A0B0C0D,
 		.cap = caps,
		.channel.send = test_pd_seq_send,
		.channel.recv = test_pd_seq_recv,
		.scbk = NULL,
	};

	struct osdp *ctx = (struct osdp *)osdp_pd_setup(&info);
	if (ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	pthread_mutex_lock(&params_mutex);
	params->pd_ctx = ctx;
	pthread_mutex_unlock(&params_mutex);
	return 0;
}

int test_cp_seq_setup(struct ThreadParams *params)
{
	/* mock application data */
	osdp_pd_info_t info = {
		.address = 101,
		.baud_rate = 115200,
		.flags = 0,
		.channel.data = NULL,
		.channel.send = test_cp_seq_send,
		.channel.recv = test_cp_seq_recv,
		.channel.flush = NULL,
		.scbk = NULL,
	};

	struct osdp *ctx = (struct osdp *)osdp_cp_setup(1, &info);
	if (ctx == NULL) {
		printf("   init failed!\n");
		return -1;
	}
	pthread_mutex_lock(&params_mutex);
	params->cp_ctx = ctx;
	pthread_mutex_unlock(&params_mutex);
	return 0;
}

void test_cp_seq_teardown(struct ThreadParams *params)
{
 	pthread_mutex_lock(&params_mutex);
	osdp_cp_teardown(params->cp_ctx);
	pthread_mutex_unlock(&params_mutex);
}

void test_pd_seq_teardown(struct ThreadParams *params)
{
	pthread_mutex_lock(&params_mutex);
	osdp_pd_teardown(params->pd_ctx);
	pthread_mutex_unlock(&params_mutex);
}


void run_cp_seq_tests(struct test *t)
{
	pthread_t pd_thread_handler;
 	struct ThreadParams params;
	struct osdp *cp_ctx;
	struct osdp *pd_ctx;
	int result = true;
	uint32_t count = 0;
   
	printf("\nStarting sequence mismatch test\n");
 	osdp_logger_init("osdp::cp", OSDP_LOG_DEBUG, NULL);

	memset(&cp_rx_buffer, 0, sizeof(cp_rx_buffer));
	memset(&pd_rx_buffer, 0, sizeof(pd_rx_buffer));

	if (test_cp_seq_setup(&params) || test_pd_seq_setup(&params))
		return;

	pthread_mutex_lock(&params_mutex);
	cp_ctx = params.cp_ctx;
 	pd_ctx = params.pd_ctx;
	pthread_mutex_unlock(&params_mutex);

	osdp_pd_set_command_callback(pd_ctx, test_pd_command_handler, NULL);
	pthread_create(&pd_thread_handler, NULL, pd_thread, (void*)&params);

	while (1) {
		osdp_cp_refresh(cp_ctx);
 		usleep(20 * 1000);

		/* send three commands, the second one would timeout */
 		if(count == 25 || count == 55 || count == 250) {
			struct osdp_cmd cmd;
			cmd.id = OSDP_CMD_MFG;
			cmd.mfg.command = 0;
			cmd.mfg.length = 10;
			osdp_cp_send_command(cp_ctx, 0, &cmd);
        	}

  		/* end test and check states */
		if (count++ > 450) {
			if (GET_CURRENT_PD(cp_ctx)->ephemeral_data[0] == OSDP_PD_NAK_SEQ_NUM ||
			    GET_CURRENT_PD(cp_ctx)->state == OSDP_CP_STATE_OFFLINE) 
				result = false;
			break;
 		}
	}
	printf("CP state: %s\n", GET_CURRENT_PD(cp_ctx)->state == OSDP_CP_STATE_ONLINE ? "online" : "offline");
	printf("Sequence number state: %s\n", GET_CURRENT_PD(cp_ctx)->ephemeral_data[0] == OSDP_PD_NAK_SEQ_NUM ? "error" : "okay");
	printf(SUB_1 "Sequence number test %s\n", result ? "succeeded" : "failed");

	TEST_REPORT(t, result);

	/* cleanup */ 
	params.keep_alive = 0;
	test_cp_seq_teardown(&params);
	test_pd_seq_teardown(&params);
	pthread_join(pd_thread_handler, NULL);
	pthread_mutex_destroy(&params_mutex);
}

