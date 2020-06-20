/* See LICENSE file for copyright and license details. */

#define TASK_LST_MAX_NUM 11
#define TASK_LST_DESC_MAX_SIZE 64
#define TASK_LST_EXPIRY 86400 /* 86400 secs = 24 hrs */

enum {
	TASK_VOID,
	TASK_TODO,
	TASK_DONE
};

typedef struct {
	int64_t expiry;
	int status[TASK_LST_MAX_NUM];
	char tdesc[TASK_LST_MAX_NUM][TASK_LST_DESC_MAX_SIZE];
} TaskLst;

void tasklst_init(TaskLst *tasks);
void tasklst_set_expiration(TaskLst *tasks);
int tasklst_expired(TaskLst tasks);
int tasklst_tasks_total(TaskLst tasks);
int tasklst_tasks_todo(TaskLst tasks);
int tasklst_tasks_done(TaskLst tasks);
int tasklst_read_from_file(TaskLst *tasks, FILE *fp);
void tasklst_write_to_file(FILE *fp, TaskLst tasks);

