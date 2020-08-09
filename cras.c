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

#define NUMARG_SIZE 5 /* 4 digits + '\0' for the numerical arg of -t/-T */

enum {
	SHORT_OUTPUT,
	LONG_OUTPUT
};

enum {
	DEF_MODE,
	APP_MODE,
	NEW_MODE,
	OUT_MODE,
	MARK_MODE
};

static void die(const char *fmt, ...);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(Task task, int i);
static void print_short_output(TaskLst list);
static void print_output(TaskLst list);
static int read_crasfile(TaskLst *list, const char *crasfile);
static void write_crasfile(const char *crasfile, TaskLst list);
static int store_input(TaskLst *list, FILE *fp);

static void usage(void);
static void input_mode(const char *crasfile, int append);
static void output_mode(const char *crasfile, int mode);
static void mark_list_mode(const char *crasfile, const char *id, int value);

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
	va_start(ap, fmt);

	printf("%s", (ansi_color != NULL) ? ansi_color : "");

	vprintf(fmt, ap);
	va_end(ap);

	printf("%s", (ansi_color != NULL) ? "\033[0m" : "");
}

static void
print_task(Task task, int i)
{
	const char *todo_color, *done_color;

	/* NO_COLOR support (https://no-color.org/) */
	if (getenv("NO_COLOR") != NULL) {
		todo_color = NULL;
		done_color = NULL;
	} else {
		todo_color = task_todo_color;
		done_color = task_done_color;
	}

	printf("#%02d ", i + 1);
	if (task.status == TASK_TODO)
		printf_color(todo_color, "%s ", task_todo_str);
	else /* TASK_DONE */
		printf_color(done_color, "%s ", task_done_str);

	printf("%s\n", task.tdesc);
}


static void
print_short_output(TaskLst list)
{
	printf("%d/%d/%d", task_lst_count_todo(list),
	       task_lst_count_done(list), task_lst_get_size(list));
}

static void
print_output(TaskLst list)
{
	int i;
	Task *ptr;

	printf("Due date: %s\n", ctime(&list.expiry));

	for(i = 0, ptr = list.first; ptr != NULL; ptr = ptr->next) {
		print_task(*ptr, i);
		++i;
	}

	if (i > 0)
		putchar('\n');

	print_short_output(list);
	printf(" to do/done/total");
}

static int
read_crasfile(TaskLst *list, const char *crasfile)
{
	int read_stat;
	FILE *fp;

	fp = fopen(crasfile, "r");
	if (errno == ENOENT)
		return -1; /* We give the chance to create the file later. */

	if (fp == NULL) {
		task_lst_cleanup(list);
		die("Could not read from %s: %s", crasfile, strerror(errno));
	}

	read_stat = task_lst_read_from_file(list, fp);
	fclose(fp);

	if (read_stat < 0) {
		task_lst_cleanup(list);
		die("Parsing error: task file corrupted.");
	}

	if (task_lst_expired(*list) > 0) {
		task_lst_cleanup(list);
		die("Due date passed (%d list overdue).", 
		    task_lst_count_todo(*list));
	}

	return 0;
}

static void
write_crasfile(const char *crasfile, TaskLst list)
{
	FILE *fp;

	if ((fp = fopen(crasfile, "w")) == NULL) {
		task_lst_cleanup(&list);
		die("Could not write to %s: %s", crasfile, strerror(errno));
	}

	task_lst_write_to_file(fp, list);
	fclose(fp);
}

static int
store_input(TaskLst *list, FILE *fp)
{
	char linebuf[TASK_LST_DESC_MAX_SIZE];

	while (feof(fp) == 0) {
		if (fgets(linebuf, TASK_LST_DESC_MAX_SIZE, fp) == NULL)
			break;

		linebuf[strlen(linebuf) - 1] = '\0'; /* Chomp '\n' */
		if (task_lst_add_task(list, TASK_TODO, linebuf) < 0)
			return -1;
	}

	return 0;
}

static void
usage(void)
{
	die("usage: cras [-anov] [-tT num] file");
}

static void
input_mode(const char *crasfile, int append)
{
	TaskLst list;

	task_lst_init(&list);
	if (append > 0)
		read_crasfile(&list, crasfile);

	if (store_input(&list, stdin) < 0) {
		task_lst_cleanup(&list);
		die("Internal memory error.");
	}

	/* Only set a new expiration date if creating a new file */
	if (append == 0)
		task_lst_set_expiration(&list, crasfile_expiry);

	write_crasfile(crasfile, list);

	task_lst_cleanup(&list);
}

static void
output_mode(const char *crasfile, int mode)
{
	TaskLst list;

	task_lst_init(&list);
	read_crasfile(&list, crasfile);

	if (mode == SHORT_OUTPUT)
		print_short_output(list);
	else
		print_output(list);

	putchar('\n');

	task_lst_cleanup(&list);
}

static void
mark_list_mode(const char *crasfile, const char *id, int value)
{
	int tasknum;
	char *endptr;
	TaskLst list;
	Task *task;

	tasknum = strtol(id, &endptr, 10);
	if (endptr[0] != '\0')
		die("'%s' not a number.", id);

	task_lst_init(&list);
	read_crasfile(&list, crasfile);

	if (tasknum <= 0) {
		task_lst_cleanup(&list);
		die("Task number must be greater than zero.");
	}

	if (tasknum <= task_lst_get_size(list))
		task = task_lst_get_task(list, tasknum - 1);
	
	if (task == NULL) {
		task_lst_cleanup(&list);
		die("Task #%d does not exist.", tasknum);
	}

	task->status = value;
	write_crasfile(crasfile, list);

	print_task(*task, tasknum - 1);

	task_lst_cleanup(&list);
}

int
main(int argc, char *argv[])
{
	char numarg[NUMARG_SIZE];
	int mode, task_value;

	mode = DEF_MODE;
	ARGBEGIN {
	case 'a':
		if (mode != DEF_MODE)
			usage();
		mode = APP_MODE;
		break;
	case 'n':
		if (mode != DEF_MODE)
			usage();
		mode = NEW_MODE;
		break;
	case 'o':
		if (mode != DEF_MODE)
			usage();
		mode = OUT_MODE;
		break;
	case 'v':
		die("Cras %s. See LICENSE file for copyright and license "
		    "details.", VERSION);
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
	default:
		usage(); /* usage() dies, so nothing else needed. */
	} ARGEND;

	if (argc <= 0)
		usage();

	switch (mode) {
	case APP_MODE:
		input_mode(argv[0], 1);
		return 0;
	case NEW_MODE:
		input_mode(argv[0], 0);
		return 0;
	case OUT_MODE:
		output_mode(argv[0], SHORT_OUTPUT);
		return 0;
	case MARK_MODE:
		mark_list_mode(argv[0], numarg, task_value);
		return 0;
	}
	
	/* Default behavior: long-form output */
	output_mode(argv[0], LONG_OUTPUT);

	return 0;
}
