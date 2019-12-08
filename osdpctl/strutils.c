/*
 * Copyright (c) 2019 Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
