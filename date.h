/* See LICENSE file for copyright and license details. */

/* 
 * "YYYY-MM-DD\0" is 11 chars, but gcc wants >36. 42 is the next multiple of 
 * 8, so using that. 
 */
#define DATE_SIZE 42

const char *date_str(time_t date);
int is_date(const char *str);
int date_cmp(const char *str);
