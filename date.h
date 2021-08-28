/* See LICENSE file for copyright and license details. */

#define DATE_SIZE 11 /* "YYYY-MM-DD\0" = 11 chars */

const char *date_str(time_t date);
int is_date(const char *str);
int date_cmp(const char *str);
