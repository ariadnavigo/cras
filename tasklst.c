/* See LICENSE file for copyright and license details. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tasklst.h"

static int tasklst_tasks_status(TaskLst tasks, int status);

static int
tasklst_tasks_status(TaskLst tasks, int status)
{
	int i, total;
	
	for (i = 0, total = 0; i < TASK_LST_MAX_NUM; ++i) {
		if (tasks.status[i] == status)
			++total;
	}

	return total;
}

void
tasklst_init(TaskLst *tasks)
{
	int i;

	tasks->expiry = 0;

	for (i = 0; i < TASK_LST_MAX_NUM; ++i) {
		memset(&tasks->status[i], TASK_VOID, sizeof(int));
		memset(tasks->tdesc[i], 0, TASK_LST_DESC_MAX_SIZE);
	}
}

void
tasklst_set_expiration(TaskLst *tasks, int64_t delta)
{
	tasks->expiry = time(NULL) + delta;
}

int
tasklst_expired(TaskLst tasks)
{
	return (time(NULL) > tasks.expiry) ? 1 : 0;
}

int
tasklst_tasks_total(TaskLst tasks)
{
	return tasklst_tasks_todo(tasks) + tasklst_tasks_done(tasks);
}

int
tasklst_tasks_todo(TaskLst tasks)
{
	return tasklst_tasks_status(tasks, TASK_TODO);
}

int
tasklst_tasks_done(TaskLst tasks)
{
	return tasklst_tasks_status(tasks, TASK_DONE);
}
		
int
tasklst_read_from_file(TaskLst *tasks, FILE *fp)
{
	int i, stat_buf;
	char *ptr, *endptr;
	char linebuf[TASK_LST_DESC_MAX_SIZE];
	
	if (fscanf(fp, "%" SCNd64 "\n", &tasks->expiry) == 0)
		return -1;

	for (i = 0; i < TASK_LST_MAX_NUM && feof(fp) == 0; ++i) {
		if (fgets(linebuf, sizeof(linebuf), fp) == NULL)
			break;

		ptr = strtok(linebuf, "\t");
		if (ptr == NULL)
			return -1;

		stat_buf = strtol(ptr, &endptr, 10);
		if (endptr[0] != '\0')
			return -1;

		if (stat_buf == TASK_VOID)
			break;
		else
			tasks->status[i] = stat_buf;

		ptr = strtok(NULL, "\n");
		if (ptr == NULL)
			return -1;

		strncpy(tasks->tdesc[i], ptr, TASK_LST_DESC_MAX_SIZE);
	}

	return 0;
}

void
tasklst_write_to_file(FILE *fp, TaskLst tasks)
{
	int i;

	fprintf(fp, "%" PRId64 "\n", tasks.expiry);

	for (i = 0; i < TASK_LST_MAX_NUM; ++i) {
		if (tasks.status[i] == TASK_VOID)
			break;

		fprintf(fp, "%d\t%s\n", tasks.status[i], tasks.tdesc[i]);
	}
}
