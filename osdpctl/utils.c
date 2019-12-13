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
 * The passed char *hstr, has to be sufficiently large.
 */
int atohstr(char *hstr, const uint8_t *arr, const int arr_len)
{
	int i, off = 0;

	for (i = 0; i < arr_len; i++)
		off += sprintf(hstr + off, "%02x", arr[i]);

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
 * uint8_t *arr has to be half of sizeof(const char *hstr)
 */
int hstrtoa(uint8_t *arr, const char *hstr)
{
	int i, len;

	len = strlen(hstr);
	if (len & 1)
		return -1; // Must have even number of chars
	len = len / 2;
	for (i = 0; i < len; i++) {
		if (sscanf(&(hstr[i * 2]), "%2hhx", &(arr[i])) != 1)
			return -1;
	}

	return 0;
}

int safe_atoi(const char *a, int *i)
{
	int val;

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
	FILE *fd;

	if (file == NULL)
		return -1;

	fd = fopen(file, "r");
	if (fd == NULL)
		return -1;

	if (fscanf(fd, "%d", pid) != 1) {
		printf("Failed to read PID from file %s\n", file);
		return -1;
	}
	fclose(fd);

	return 0;
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
