/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note:
 *   This code is based on Teunis van Beelen's implementation. Its API and coding
 *   style needed a lot of work before consumption and hence warrented a full
 *   rewrite. His implementation can be found at https://www.teuniz.net/RS-232/
 *
 *   A good explanation can be found at https://www.cmrr.umn.edu/~strupp/serial.html
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>

#include "serial.h"

struct serial *serial_open(const char *device, int baud, const char *mode)
{
	int status, tmp;
	int flow_control = 0, cflags = 0, ignore_parity = 0;
	struct serial *ctx;

	switch (baud) {
	case 50:	baud = B50; break;
	case 75:	baud = B75; break;
	case 110:	baud = B110; break;
	case 134:	baud = B134; break;
	case 150:	baud = B150; break;
	case 200:	baud = B200; break;
	case 300:	baud = B300; break;
	case 600:	baud = B600; break;
	case 1200:	baud = B1200; break;
	case 1800:	baud = B1800; break;
	case 2400:	baud = B2400; break;
	case 4800:	baud = B4800; break;
	case 9600:	baud = B9600; break;
	case 19200:	baud = B19200; break;
	case 38400:	baud = B38400; break;
	case 57600:	baud = B57600; break;
	case 115200:	baud = B115200; break;
	case 230400:	baud = B230400; break;
	default:
		printf("invalid baudrate\n");
		return NULL;
	}

	tmp = strnlen(mode, 5);
	if (tmp < 3 || tmp > 4) {
		printf("invalid mode \"%s\"\n", mode);
		return NULL;
	}

	if (tmp == 4 && (mode[3] == 'F' || mode[3] == 'f'))
		flow_control = 1;

	switch (mode[0]) {
	case '8': cflags |= CS8; break;
	case '7': cflags |= CS7; break;
	case '6': cflags |= CS6; break;
	case '5': cflags |= CS5; break;
	default:
		printf("invalid number of data-bits '%c'\n", mode[0]);
		return  NULL;
	}

	if (mode[1] == 'N' || mode[1] == 'n') { /* no parity */
		cflags |= IGNPAR;
		ignore_parity = 1;
	} else { /* has parity */
		cflags |= PARENB;
		if (mode[1] == 'O' || mode[1] == 'o')
			cflags |= PARODD;
	}

	if (mode[2] == '2') {
		cflags |= CSTOPB;
	}

	ctx = calloc(1, sizeof(struct serial));
	if (ctx == NULL) {
		printf("failed to alloc struct serial_port\n");
		return NULL;
	}

	ctx->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (ctx->fd == -1) {
		perror("unable to open comport");
		goto error;
	}

	if (flock(ctx->fd, LOCK_EX | LOCK_NB) != 0) {
		close(ctx->fd);
		perror("another process has locked the comport");
		goto error;
	}

	if (tcgetattr(ctx->fd, &ctx->old_termios) == -1) {
		close(ctx->fd);
		perror("unable to read portsettings");
		goto error;
	}

	ctx->new_termios.c_cflag = cflags | CLOCAL | CREAD;
	if (flow_control)
		ctx->new_termios.c_cflag |= CRTSCTS;
	ctx->new_termios.c_iflag = ignore_parity ? IGNPAR : INPCK;
	ctx->new_termios.c_oflag = 0;
	ctx->new_termios.c_lflag = 0;

	/* block until n bytes are received */
	ctx->new_termios.c_cc[VMIN] = 0;

	/* block until a timer expires (n * 100 mSec.) */
	ctx->new_termios.c_cc[VTIME] = 0;

	cfsetispeed(&ctx->new_termios, baud);
	cfsetospeed(&ctx->new_termios, baud);

	if (tcsetattr(ctx->fd, TCSANOW, &ctx->new_termios) == -1) {
		tcsetattr(ctx->fd, TCSANOW, &ctx->old_termios);
		close(ctx->fd);
		perror("unable to adjust portsettings");
		goto error;
	}

	if (ioctl(ctx->fd, TIOCMGET, &status) == -1) {
		tcsetattr(ctx->fd, TCSANOW, &ctx->old_termios);
		perror("unable to get portstatus");
		goto error;
	}

	status |= TIOCM_DTR; /* turn on DTR */
	status |= TIOCM_RTS; /* turn on RTS */

	if (ioctl(ctx->fd, TIOCMSET, &status) == -1) {
		tcsetattr(ctx->fd, TCSANOW, &ctx->old_termios);
		perror("unable to set portstatus");
		goto error;
	}

	return ctx;

error:
	if (ctx->fd != 0 && ctx->fd != -1)
		flock(ctx->fd, LOCK_UN);
	free(ctx);
	return NULL;
}

void serial_close(struct serial *ctx)
{
	int status;

	if (ctx == NULL)
		return;

	if (ioctl(ctx->fd, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}

	status &= ~TIOCM_DTR; /* turn off DTR */
	status &= ~TIOCM_RTS; /* turn off RTS */

	if (ioctl(ctx->fd, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}

	tcsetattr(ctx->fd, TCSANOW, &ctx->old_termios);
	close(ctx->fd);
	flock(ctx->fd, LOCK_UN);

	free(ctx);
}

int serial_read(struct serial *ctx, unsigned char *buf, int size)
{
	int n;

	n = read(ctx->fd, buf, size);
	if (n < 0) {
		if (errno == EAGAIN)
			return 0;
	}
	return n;
}

int serial_write(struct serial *ctx, unsigned char *buf, int size)
{
	int n;

	n = write(ctx->fd, buf, size);
	if (n < 0) {
		if (errno == EAGAIN)
			return 0;
	}
	return n;
}

void serial_flush_rx(struct serial *ctx)
{
	tcflush(ctx->fd, TCIFLUSH);
}

void serial_flush_tx(struct serial *ctx)
{
	tcflush(ctx->fd, TCOFLUSH);
}

void serial_flush(struct serial *ctx)
{
	tcflush(ctx->fd, TCIOFLUSH);
}

int serial_get_dcd(struct serial *ctx)
{
	int status;

	ioctl(ctx->fd, TIOCMGET, &status);

	return (status & TIOCM_CAR) ? 1 : 0;
}

int serial_get_rng(struct serial *ctx)
{
	int status;

	ioctl(ctx->fd, TIOCMGET, &status);

	return (status & TIOCM_RNG) ? 1 : 0;
}

int serial_get_cts(struct serial *ctx)
{
	int status;

	ioctl(ctx->fd, TIOCMGET, &status);

	return (status & TIOCM_CTS) ? 1 : 0;
}

int serial_get_dsr(struct serial *ctx)
{
	int status;

	ioctl(ctx->fd, TIOCMGET, &status);

	return (status & TIOCM_DSR) ? 1 : 0;
}

void serial_assert_dtr(struct serial *ctx, int state)
{
	int status;

	if (ioctl(ctx->fd, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}

	if (state)
		status |= TIOCM_DTR; /* turn on RTS */
	else
		status &= ~TIOCM_DTR; /* turn off RTS */

	if (ioctl(ctx->fd, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}

void serial_assert_rts(struct serial *ctx, int state)
{
	int status;

	if (ioctl(ctx->fd, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}

	if (state)
		status |= TIOCM_RTS; /* turn on RTS */
	else
		status &= ~TIOCM_RTS; /* turn off RTS */

	if (ioctl(ctx->fd, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}
