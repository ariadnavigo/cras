/* See LICENSE file for copyright and license details. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "strlcpy.h"
#include "tasklst.h"

static Task *task_lst_get_last_task(TaskLst list);

static Task *
task_lst_get_last_task(TaskLst list)
{
	Task *ptr, *next;

	ptr = list.first;
	while (ptr != NULL) {
		next = ptr->next;
		if (next == NULL)
			return ptr;

		ptr = next;
	}

	return ptr;
}

size_t
task_set_tdesc(Task *task, const char *str)
{
	size_t retval;

	retval = strlcpy(task->tdesc, str, TASK_TDESC_SIZE);
	if (task->tdesc[strlen(task->tdesc) - 1] == '\n')
		task->tdesc[strlen(task->tdesc) - 1] = '\0';

	return retval;
}

void
task_lst_init(TaskLst *list)
{
	list->first = NULL;
}

void
task_lst_cleanup(TaskLst *list)
{
	Task *ptr, *next;

	ptr = list->first;
	while (ptr != NULL) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
}

void
task_lst_set_date(TaskLst *list)
{
	time_t utim;
	struct tm *ltim;

	utim = time(NULL);
	ltim = localtime(&utim);
	snprintf(list->date, TASK_DATE_SIZE, "%04d-%02d-%02d", 
                 ltim->tm_year + 1900, ltim->tm_mon + 1, ltim->tm_mday);
}

int
task_lst_on_date(TaskLst list)
{
	struct tm *ltim;
	time_t utim;
	int year, month, day;
	
	utim = time(NULL);
	ltim = localtime(&utim);
	sscanf(list.date, "%d-%d-%d", &year, &month, &day);

	/* We are valid only if on the same day */
	if ((year == ltim->tm_year + 1900) && (month == ltim->tm_mon + 1) 
	    && (day == ltim->tm_mday))
		return 0;
	else
		return -1;
}

Task *
task_lst_get_task(TaskLst list, int i)
{
	Task *ptr;

	for (ptr = list.first; i > 0; ptr = ptr->next) {
		if (ptr == NULL) /* We're out of bounds */
			return NULL;

		--i;
	}

	return ptr;
}

int
task_lst_add_task(TaskLst *list, int status, const char *str)
{
	Task *last, *newtask;

	newtask = malloc(sizeof(Task));
	if (newtask == NULL)
		return -1;

	newtask->status = status;
	task_set_tdesc(newtask, str);

	last = task_lst_get_last_task(*list);
	if (last == NULL)
		list->first = newtask;
	else
		last->next = newtask;

	newtask->prev = last; /* Also if last == NULL */
	newtask->next = NULL;

	return 0;
}

int
task_lst_del_task(TaskLst *list, int i)
{
	Task *del, *prev, *next;

	if ((del = task_lst_get_task(*list, i)) == NULL)
		return -1;

	prev = del->prev;
	next = del->next;

	if (prev != NULL)
		prev->next = next;

	if (next != NULL)
		next->prev = prev;

	if (list->first == del)
		list->first = next;

	free(del);

	return 0;
}

int
task_lst_read_from_file(TaskLst *list, FILE *fp)
{
	int stat_buf;
	char *ptr, *endptr;
	char linebuf[TASK_LST_BUF_SIZE];

	task_lst_init(list);

	if (fscanf(fp, "%s\n", list->date) <= 0)
		return -1;

	while (feof(fp) == 0) {
		if (fgets(linebuf, TASK_LST_BUF_SIZE, fp) == NULL)
			break;

		ptr = strtok(linebuf, "\t");
		if (ptr == NULL)
			return -1;

		stat_buf = strtol(ptr, &endptr, 10);
		if (endptr[0] != '\0')
			return -1;

		ptr = strtok(NULL, "\n");
		if (ptr == NULL)
			return -1;

		task_lst_add_task(list, stat_buf, ptr);
	}

	return 0;
}

void
task_lst_write_to_file(FILE *fp, TaskLst list)
{
	Task *ptr;

	fprintf(fp, "%s\n", list.date);

	for (ptr = list.first; ptr != NULL; ptr = ptr->next)
		fprintf(fp, "%d\t%s\n", ptr->status, ptr->tdesc);
}
