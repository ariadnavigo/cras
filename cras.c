/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char *argv0; /* Required here by arg.h */
#include "arg.h"
#include "config.h"
#include "tasklst.h"

#define TASK_NONEXIST_MSG "Task #%d does not exist."
#define NUMARG_SIZE 5 /* 4 digits + '\0' for arg of -d/-e/-t/-T */

enum {
	APP_MODE,
	DEF_MODE,
	DLT_MODE,
	EDIT_MODE,
	INVAL_MODE,
	LONG_OUT_MODE,
	MARK_MODE,
	NEW_MODE,
	SHORT_DESC_OUT_MODE,
	SHORT_OUT_MODE,
	TASKS_OUT_MODE
};

/* Auxiliary functions */
static void die(const char *fmt, ...);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(Task task, int i);
static int print_task_list(TaskLst list);
static void print_counter(TaskLst list);
static void read_file(TaskLst *list, const char *fname);
static void write_file(const char *fname, TaskLst list);
static int store_input(TaskLst *list, FILE *fp);
static int parse_tasknum(const char *id);
static void usage(void);

/* Execution modes */
static void delete_mode(const char *fname, const char *id);
static void edit_mode(const char *fname, const char *id);
static void input_mode(const char *fname, int append);
static void inval_mode(const char *fname);
static void mark_list_mode(const char *fname, const char *id, int value);
static void output_mode(const char *fname, int mode);

static void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);

	va_end(ap);

	exit(1);
}

static void
printf_color(const char *ansi_color, const char *fmt, ...)
{
	va_list ap;
	const char *eff_color; /* "Effective color" */

	/* NO_COLOR support (https://no-color.org/) */
	if (getenv("NO_COLOR") != NULL) {
		eff_color = NULL;
	} else {
		eff_color = ansi_color;
	}

	printf("%s", (eff_color != NULL) ? eff_color : "");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("%s", (eff_color != NULL) ? "\033[0m" : "");
}

static void
print_task(Task task, int i)
{
	printf("#%02d ", i + 1);
	if (task.status == TASK_TODO)
		printf_color(task_todo_color, "%s ", task_todo_str);
	else /* TASK_DONE */
		printf_color(task_done_color, "%s ", task_done_str);

	printf("%s\n", task.tdesc);
}

static int
print_task_list(TaskLst list)
{
	Task *ptr;
	int i;

	for(i = 0, ptr = list.first; ptr != NULL; ptr = ptr->next) {
		print_task(*ptr, i);
		++i;
	}

	return i;
}

static void
print_counter(TaskLst list)
{
	printf_color(task_todo_color, "%d", task_lst_count_todo(list));
	printf("/");
	printf_color(task_done_color, "%d", task_lst_count_done(list));
}

static void
read_file(TaskLst *list, const char *fname)
{
	int read_stat;
	FILE *fp;

	fp = fopen(fname, "r");
	if (fp == NULL)
		die("Could not read from %s: %s", fname, strerror(errno));

	read_stat = task_lst_read_from_file(list, fp);
	fclose(fp);

	if (read_stat < 0) {
		task_lst_cleanup(list);
		die("Parsing error: task file corrupted.");
	}

	if (task_lst_expired(*list) > 0) {
		task_lst_cleanup(list);
		die("Due date passed."); 
	}
}

static void
write_file(const char *fname, TaskLst list)
{
	FILE *fp;

	if ((fp = fopen(fname, "w")) == NULL) {
		task_lst_cleanup(&list);
		die("Could not write to %s: %s", fname, strerror(errno));
	}

	task_lst_write_to_file(fp, list);
	fclose(fp);
}

static int
store_input(TaskLst *list, FILE *fp)
{
	int cnt;
	char linebuf[TASK_LST_DESC_MAX_SIZE];

	cnt = 0;
	while (feof(fp) == 0) {
		if (fgets(linebuf, TASK_LST_DESC_MAX_SIZE, fp) == NULL)
			break;

		/* Chomp '\n' */
		if (linebuf[strlen(linebuf) - 1] == '\n')
			linebuf[strlen(linebuf) - 1] = '\0';

		/* 
		 * Ignoring blank lines so that the file doesn't get corrupted
		 * by one. We calculate strlen(linebuf) again because we 
		 * *might* have chomped '\n' or not. Storing the size 
		 * beforehand is not a viable optimization, as far as I can 
		 * see.
		 */
		if (strlen(linebuf) == 0)
			continue;

		if (task_lst_add_task(list, TASK_TODO, linebuf) < 0)
			return -1;

		++cnt;
	}

	return cnt;
}

static int
parse_tasknum(const char *id)
{
	int tasknum;
	char *endptr;

	tasknum = strtol(id, &endptr, 10);
	if (endptr[0] != '\0')
		die("'%s' not a number.", id);
	if (tasknum <= 0)
		die("Task number must be greater than zero.");

	return tasknum;
}

static void
usage(void)
{
	die("usage: cras [-ailnoOv] [-detT num] [file]");
}

static void
delete_mode(const char *fname, const char *id)
{
	int tasknum;
	TaskLst list;

	tasknum = parse_tasknum(id);

	read_file(&list, fname);

	if (task_lst_del_task(&list, tasknum - 1) < 0) {
		task_lst_cleanup(&list);
		die(TASK_NONEXIST_MSG, tasknum);
	}
	write_file(fname, list);

	task_lst_cleanup(&list);
}

static void
edit_mode(const char *fname, const char *id)
{
	int tasknum;
	TaskLst list;
	char newstr[TASK_LST_DESC_MAX_SIZE];

	tasknum = parse_tasknum(id);

	read_file(&list, fname);

	fgets(newstr, TASK_LST_DESC_MAX_SIZE, stdin);
	if (newstr[strlen(newstr) - 1] == '\n')
		newstr[strlen(newstr) - 1] = '\0';

	if (task_lst_edit_task(&list, tasknum - 1, newstr) < 0) {
		task_lst_cleanup(&list);
		die(TASK_NONEXIST_MSG, tasknum);
	}
	write_file(fname, list);

	task_lst_cleanup(&list);
}

static void
input_mode(const char *fname, int append)
{
	int task_num;
	TaskLst list;

	if (append > 0)
		read_file(&list, fname);
	else
		task_lst_init(&list);

	task_num = store_input(&list, stdin);
	if (task_num < 0) {
		task_lst_cleanup(&list);
		die("Internal memory error.");
	} else if (task_num == 0) {
		task_lst_cleanup(&list);
		die("Aborting: empty task list.");
	} else {
		/* Only set a new expiration date if creating a new file */
		if (append == 0)
			task_lst_set_expiration(&list, expiry_delta);

		write_file(fname, list);

		task_lst_cleanup(&list);
	}
}

static void
inval_mode(const char *fname)
{
	TaskLst list;

	read_file(&list, fname);

	task_lst_set_expiration(&list, 0);
	write_file(fname, list);

	task_lst_cleanup(&list);
}

static void
mark_list_mode(const char *fname, const char *id, int value)
{
	int tasknum;
	TaskLst list;
	Task *task;

	tasknum = parse_tasknum(id);

	read_file(&list, fname);

	task = task_lst_get_task(list, tasknum - 1);
	if (task == NULL) {
		task_lst_cleanup(&list);
		die(TASK_NONEXIST_MSG, tasknum);
	}

	task->status = value;
	write_file(fname, list);

	print_task(*task, tasknum - 1);

	task_lst_cleanup(&list);
}

static void
output_mode(const char *fname, int mode)
{
	TaskLst list;
	int cnt;

	read_file(&list, fname);

	if (mode == SHORT_OUT_MODE) {
		print_counter(list);
		putchar('\n');
	} else if (mode == SHORT_DESC_OUT_MODE) {
		print_counter(list);
		printf(" %s\n", smmry_str);
	} else if (mode == TASKS_OUT_MODE) {
		print_task_list(list);
	} else { 
		/* LONG_OUT_MODE */
		printf("%s: %s\n", expire_str, ctime(&list.expiry));
		cnt = print_task_list(list);
		if (cnt > 0) 
			putchar('\n');
		print_counter(list);
		printf(" %s\n", smmry_str);
	}

	task_lst_cleanup(&list);
}

int
main(int argc, char *argv[])
{
	char numarg[NUMARG_SIZE];
	const char *fileptr;
	int mode, task_value;

	mode = DEF_MODE;
	ARGBEGIN {
	case 'a':
		if (mode != DEF_MODE)
			usage();
		mode = APP_MODE;
		break;
	case 'd':
		if (mode != DEF_MODE)
			usage();
		mode = DLT_MODE;
		strncpy(numarg, EARGF(usage()), NUMARG_SIZE);
		break;
	case 'e':
		if (mode != DEF_MODE)
			usage();
		mode = EDIT_MODE;
		strncpy(numarg, EARGF(usage()), NUMARG_SIZE);
		break;
	case 'i':
		if (mode != DEF_MODE)
			usage();
		mode = INVAL_MODE;
		break;
	case 'l':
		if (mode != DEF_MODE)
			usage();
		mode = TASKS_OUT_MODE;
		break;
	case 'n':
		if (mode != DEF_MODE)
			usage();
		mode = NEW_MODE;
		break;
	case 'o':
		if (mode != DEF_MODE)
			usage();
		mode = SHORT_OUT_MODE;
		break;
	case 'O':
		if (mode != DEF_MODE)
			usage();
		mode = SHORT_DESC_OUT_MODE;
		break;
	case 't':
		if (mode != DEF_MODE)
			usage();
		mode = MARK_MODE;
		task_value = TASK_DONE;
		strncpy(numarg, EARGF(usage()), NUMARG_SIZE);
		break;
	case 'T':
		if (mode != DEF_MODE)
			usage();
		mode = MARK_MODE;
		task_value = TASK_TODO;
		strncpy(numarg, EARGF(usage()), NUMARG_SIZE);
		break;
	case 'v':
		die("cras %s", VERSION);
		break;
	default:
		usage(); /* usage() dies, so nothing else needed. */
	} ARGEND;

	if (argc <= 0) {
		if ((fileptr = getenv("CRAS_DEF_FILE")) == NULL)
			die("CRAS_DEF_FILE environment variable not set.");
	} else {
		fileptr = argv[0];
	}

	switch (mode) {
	case APP_MODE:
		input_mode(fileptr, 1);
		return 0;
	case DLT_MODE:
		delete_mode(fileptr, numarg);
		return 0;
	case EDIT_MODE:
		edit_mode(fileptr, numarg);
		return 0;
	case INVAL_MODE:
		inval_mode(fileptr);
		return 0;
	case MARK_MODE:
		mark_list_mode(fileptr, numarg, task_value);
		return 0;
	case NEW_MODE:
		input_mode(fileptr, 0);
		return 0;
	default:
		output_mode(fileptr, mode);
		return 0;
	}

	/* Default behavior: long-form output */
	output_mode(fileptr, LONG_OUT_MODE);

	return 0;
}
