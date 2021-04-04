/* See LICENSE file for copyright and license details. */

#define TASK_TDESC_SIZE 64

/* Let's give buffers some breathing room, just to be sure. */
#define TASK_LST_BUF_SIZE TASK_TDESC_SIZE + 10

enum {
	TASK_TODO,
	TASK_DONE
};

typedef struct TASK_ Task;
struct TASK_ {
	int status;
	Task *prev;
	Task *next;
	char tdesc[TASK_TDESC_SIZE];
};

typedef struct {
	int64_t expiry;
	Task *first;
} TaskLst;

size_t task_set_tdesc(Task *task, const char *str);
void task_lst_init(TaskLst *list);
void task_lst_cleanup(TaskLst *list);
void task_lst_set_expiration(TaskLst *list, int64_t delta);
int task_lst_expired(TaskLst list);
int task_lst_count_status(TaskLst list, int status);
Task *task_lst_get_task(TaskLst list, int i);
int task_lst_add_task(TaskLst *list, int status, const char *str);
int task_lst_del_task(TaskLst *list, int i);
int task_lst_read_from_file(TaskLst *list, FILE *fp);
void task_lst_write_to_file(FILE *fp, TaskLst list);

