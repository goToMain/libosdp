#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>

#ifndef BUILD_ZEPHYR
#error "Do not compile this file when not building for zephyr"
#endif

struct device *uart_dev;

void osdp_uart_isr(struct device *dev)
{
	int rx;
	struct osdp_pd *pd = g_osdp_context.pd;

	k_spinlock_key_t key = k_spin_lock(&pd->rx_lock);
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (!uart_irq_rx_ready(dev))
			continue;

		/* Character(s) have been received */

		if (pd->rx_len > OSDP_PD_RX_BUF_LENGTH)
			return;

		rx = uart_fifo_read(dev, pd->rx_data + pd->rx_len,
				    OSDP_PD_RX_BUF_LENGTH - pd->rx_len);
		if (rx  <  0)
			continue;

		pd->rx_len += rx;
	}
	k_spin_unlock(&pd->rx_lock, key);
}

void osdp_uart_init()
{
	/* OSDP UART init */
	u8_t c;
	pd->uart_dev =  device_get_binding(CONFIG_OSDP_UART_DEV_NAME);
	uart_irq_rx_disable(pd->uart_dev);
	uart_irq_tx_disable(pd->uart_dev);
	uart_irq_callback_set(pd->uart_dev, osdp_uart_isr);
	/* Drain the fifo */
	while (uart_irq_rx_ready(pd->uart_dev)) {
		uart_fifo_read(pd->uart_dev, &c, 1);
	}
	uart_irq_rx_enable(pd->uart_dev);
}

int osdp_uart_send(struct osdp_pd *p, u8_t *buf, int len)
{
	int sent = 0;

	while (sent < len) {
		uart_poll_out(p->uart_dev, buf[sent]);
		sent++;
	}

	return sent;
}

struct osdp_pd *osdp_zephyr_init()
{
	struct osdp_pd *ctx;
	struct pd_cap cap[] = {
		{
			.function_code = CAP_READER_LED_CONTROL,
			.compliance_level = 1,
			.num_items = 1
		},
		{
			.function_code = CAP_READER_AUDIBLE_OUTPUT,
			.compliance_level = 1,
			.num_items = 1
		},
		OSDP_PD_CAP_SENTINEL
	};

	osdp_pd_info_t info_pd = {
		.address = 101,
		.baud_rate = 9600,
		.init_flags = 0,
		.send_func = sample_pd_send_func,
		.recv_func = sample_pd_recv_func,
		.id = {
			.version = 1,
			.model = 153,
			.vendor_code = 31337,
			.serial_number = 0x01020304,
			.firmware_version = 0x0A0B0C0D,
		},
		.cap = cap,
	};

	ctx = osdp_pd_setup(&info_pd);
	if (ctx == NULL) {
		printf("pd init failed!\n");
		return -1;
	}
}
