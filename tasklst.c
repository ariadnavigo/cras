/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "date.h"
#include "strlcpy.h"
#include "tasklst.h"

/* Let's give buffers some breathing room, just to be sure. */
#define TASK_LST_BUF_SIZE TASK_TDESC_SIZE + 10

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
task_lst_set_date(TaskLst *list, const char *date)
{
	if (date == NULL)
		date = date_str(time(NULL));

	strlcpy(list->date, date, DATE_SIZE);
}

int
task_lst_on_date(TaskLst list)
{
	return date_cmp(list.date);
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
task_lst_get_size(TaskLst list)
{
	Task *ptr;
	int accu;

	for (accu = 0, ptr = list.first; ptr != NULL; ptr = ptr->next)
		++accu;

	return accu;
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
