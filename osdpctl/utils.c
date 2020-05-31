/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static inline int hex2int(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	ch &= ~0x20; // to upper case.
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

static inline char int2hex(int v)
{
	v &= 0x0f; // consider only the lower nibble
	return (v >= 0 && v <= 9) ? '0' + v : 'A' + v;
}

/*
 * atohstr - Array to Hex String
 *
 * Usage:
 *   uint8_t arr[4] = { 0xca, 0xfe, 0xba, 0xbe };
 *   char hstr[16];
 *   if (atohstr(hstr, arr, 4) == 0) {
 *       // hstr = "cafebabe\0";
 *   }
 *
 * Note:
 * The passed char *hstr, has to be 2 * arr_len.
 */
int atohstr(char *hstr, const uint8_t *arr, const int arr_len)
{
	int i;

	for (i = 0; i < arr_len; i++) {
		hstr[(i * 2) + 0] = int2hex(arr[i] >> 4);
		hstr[(i * 2) + 1] = int2hex(arr[i]);
	}

	return 0;
}

/*
 * hstrtoa - Hex String to Array
 *
 * Usage:
 *   uint8_t arr[4];
 *   const char *hstr = "cafebabe";
 *   if (hstrtoa(arr, hstr) == 0) {
 *       // arr[4] = { 0xca, 0xfe, 0xba, 0xbe };
 *   }
 *
 * Note:
 * uint8_t *arr has to be atleast half of strlen(hstr)
 */
int hstrtoa(uint8_t *arr, const char *hstr)
{
	int i, len, nib1, nib2;

	len = strlen(hstr);
	if ((len & 1) == 1 || len == 0)
		return -1; // Must have even number of chars
	len = len / 2;
	for (i = 0; i < len; i++) {
		nib1 = hex2int(hstr[(i * 2) + 0]);
		nib2 = hex2int(hstr[(i * 2) + 1]);
		if (nib1 == -1 || nib2 == -1)
			return -1;
		arr[i] = nib1 << 4 | nib2;
	}

	return len;
}

int safe_atoi(const char *a, int *i)
{
	int val;

	if (a == NULL)
		return -1;
	val = atoi(a);
	if (val == 0 && a[0] != '0')
		return -1;
	*i = val;
	return 0;
}

void remove_spaces(char *str)
{
	int i, j = 0;

	for (i = 0; str[i]; i++)
		if (str[i] != ' ')
			str[j++] = str[i];
	str[j] = '\0';
}

int read_pid(const char *file, int *pid)
{
	int ret = 0;
	FILE *fd;

	if (file == NULL)
		return -1;

	fd = fopen(file, "r");
	if (fd == NULL)
		return -1;

	if (fscanf(fd, "%d", pid) != 1) {
		printf("Failed to read PID from file %s\n", file);
		ret = -1;
	}
	fclose(fd);

	return ret;
}

int write_pid(const char *file)
{
	FILE *fd;
	char pid_str[32];

	if (file == NULL)
		return 1;

	fd = fopen(file, "w");
	if (fd == NULL) {
		perror("Error opening pid_file to write");
		return -1;
	}
	snprintf(pid_str, 32, "%d\n", getpid());
	fputs(pid_str, fd);
	fclose(fd);

	return 0;
}

int redirect_output_to_log_file(const char *file)
{
	int log_file = open(file, O_RDWR | O_CREAT | O_APPEND, 0600);
	if (log_file == -1) {
		perror("opening log_file");
		return -1;
	}
	if (dup2(log_file, fileno(stdout)) == -1) {
		perror("cannot redirect stdout to log_file");
		return -1;
	}
	if (dup2(log_file, fileno(stderr)) == -1) {
		perror("cannot redirect stderr to log_file");
		return -1;
	}
	return 0;
}
