/* See LICENSE file for copyright and license details. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tasklst.h"

static int task_lst_count_status(TaskLst list, int status);
static Task *task_lst_get_last_task(TaskLst list);

static int
task_lst_count_status(TaskLst list, int status)
{
	int total;
	Task *ptr;
	
	for (ptr = list.first, total = 0; ptr != NULL; ptr = ptr->next) {
		if (ptr->status == status)
			++total;
	}

	return total;
}

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

void
task_lst_init(TaskLst *list)
{
	list->expiry = 0;
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

int
task_lst_get_size(TaskLst list)
{
	Task *ptr;
	int count;

	for (count = 0, ptr = list.first; ptr != NULL; ptr = ptr->next)
		++count;

	return count;
}

void
task_lst_set_expiration(TaskLst *list, int64_t delta)
{
	list->expiry = time(NULL) + delta;
}

int
task_lst_expired(TaskLst list)
{
	return (time(NULL) > list.expiry) ? 1 : 0;
}

int
task_lst_count_todo(TaskLst list)
{
	return task_lst_count_status(list, TASK_TODO);
}

int
task_lst_count_done(TaskLst list)
{
	return task_lst_count_status(list, TASK_DONE);
}

int
task_lst_add_task(TaskLst *list, int status, const char *str)
{
	Task *last, *newtask;

	newtask = malloc(sizeof(Task));
	if (newtask == NULL)
		return -1;

	newtask->status = status;
	strncpy(newtask->tdesc, str, TASK_LST_DESC_MAX_SIZE);

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

	if ((i > task_lst_get_size(*list)) || (i < 0))
		return -1;

	if ((del = task_lst_get_task(*list, i)) == NULL)
		return -1;

	prev = del->prev;
	next = del->next;

	if (prev != NULL)
		prev->next = next;

	if (next != NULL)
		next->prev = prev;

	free(del);

	return 0;
}

Task *
task_lst_get_task(TaskLst list, int i) /* TODO: mv above task_lst_add_task() */
{
	/* TODO: Maybe binary search? */

	Task *ptr;
	
	for (ptr = list.first; i > 0; ptr = ptr->next) {
		if (ptr == NULL) /* We're out of bounds */
			return NULL;

		--i;
	}

	return ptr;
}

int
task_lst_read_from_file(TaskLst *list, FILE *fp)
{
	int stat_buf;
	char *ptr, *endptr;
	char linebuf[TASK_LST_DESC_MAX_SIZE];
	
	if (fscanf(fp, "%" SCNd64 "\n", &list->expiry) <= 0)
		return -1;

	while (feof(fp) == 0) {
		if (fgets(linebuf, sizeof(linebuf), fp) == NULL)
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

	fprintf(fp, "%" PRId64 "\n", list.expiry);

	for (ptr = list.first; ptr != NULL; ptr = ptr->next)
		fprintf(fp, "%d\t%s\n", ptr->status, ptr->tdesc);
}
