/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "date.h"
#include "strlcpy.h"

const char *
date_str(time_t date)
{
	static char static_date[DATE_SIZE];
	struct tm *time_info;

	time_info = localtime(&date);

	sprintf(static_date, "%04d-%02d-%02d", time_info->tm_year + 1900,
	        time_info->tm_mon + 1, time_info->tm_mday);

	return static_date;
}

int
is_date(const char *str)
{
	char copy[DATE_SIZE];
	char *ptr, *endptr;
	int ybuf, mbuf, dbuf, leap;

	strlcpy(copy, str, DATE_SIZE);

	/* Year */

	if ((ptr = strtok(copy, "-")) == NULL)
		return -1;

	if (strlen(ptr) != 4)
		return -1;

	ybuf = strtol(ptr, &endptr, 10);
	if (endptr[0] != '\0')
		return -1;

	if (ybuf % 400 == 0)
		leap = 1;
	else if (ybuf % 4 == 0 && ybuf % 100 != 0)
		leap = 1;
	else
		leap = 0;

	/* Month */

	if ((ptr = strtok(NULL, "-")) == NULL)
		return -1;

	if (strlen(ptr) != 2)
		return -1;

	mbuf = strtol(ptr, &endptr, 10);
	if (endptr[0] != '\0')
		return -1;

	if (mbuf < 1 || mbuf > 12)
		return -1;

	/* Day */

	if ((ptr = strtok(NULL, "")) == NULL)
		return -1;

	if (strlen(ptr) != 2)
		return -1;

	dbuf = strtol(ptr, &endptr, 10);
	if (endptr[0] != '\0')
		return -1;

	if (dbuf < 0)
		return -1;
	if (leap > 0 && mbuf == 2 && dbuf == 29)
		return 0;
	else if (leap == 0 && mbuf == 2 && dbuf > 28)
		return -1;
	else if ((mbuf == 1 || mbuf == 3 || mbuf == 5 || mbuf == 7 || mbuf == 8
	          || mbuf == 10 || mbuf == 12) && dbuf > 31)
		return -1;
	else if ((mbuf == 4 || mbuf == 6 || mbuf == 9 || mbuf == 11)
	         && dbuf > 30)
		return -1;
	else
		return 0;
}

int
date_cmp(const char *str)
{
	struct tm *ltim;
	time_t utim;
	int year, month, day, cmp_year, cmp_month, cmp_day, cmp_ymd;

	utim = time(NULL);
	ltim = localtime(&utim);
	sscanf(str, "%d-%d-%d", &year, &month, &day);

	cmp_year = (year == ltim->tm_year + 1900 ? 0
	            : (year > ltim->tm_year + 1900 ? 4 : -4));
	cmp_month = (month == ltim->tm_mon + 1 ? 0
	             : (month > ltim->tm_mon + 1 ? 2 : -2));
	cmp_day = (day == ltim->tm_mday ? 0
	           : (day > ltim->tm_mday ? 1 : -1));
	/* It's a hack but the sum should indicate whether
	 * the date is in future or not. */
	cmp_ymd = cmp_year + cmp_month + cmp_day;

	/* We are valid only if on the same day */
	if (cmp_ymd == 0)
		return 0;
	else if (cmp_ymd > 0)
		return 1; /* The list's date is in the future. */
	else
		return -1; /* The list's date is in the past. */
}
