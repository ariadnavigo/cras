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
	NEW_MODE,
	OUT_MODE,
	MARK_MODE
};

static void die(const char *fmt, ...);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(TaskLst tasks, int i);
static void print_short_output(TaskLst tasks);
static void print_output(TaskLst tasks);
static int read_crasfile(TaskLst *tasks, const char *crasfile);
static void write_crasfile(const char *crasfile, TaskLst tasks);
static int store_input(TaskLst *tasks, FILE *fp);

static void usage(void);
static void new_mode(const char *crasfile);
static void output_mode(const char *crasfile, int mode);
static void mark_tasks_mode(const char *crasfile, const char *id, int value);

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
print_task(TaskLst tasks, int i)
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

	/* Null tasks are never printed */
	if (tasks.status[i] == TASK_VOID)
		return;

	printf("#%02d ", i + 1);
	if (tasks.status[i] == TASK_TODO)
		printf_color(todo_color, "%s ", task_todo_str);
	else /* TASK_DONE */
		printf_color(done_color, "%s ", task_done_str);

	printf("%s\n", tasks.tdesc[i]);
}


static void
print_short_output(TaskLst tasks)
{
	printf("%d/%d/%d", tasklst_tasks_todo(tasks),
	       tasklst_tasks_done(tasks), tasklst_tasks_total(tasks));
}

static void
print_output(TaskLst tasks)
{
	int i;

	printf("Due date: %s\n", ctime(&tasks.expiry));

	for(i = 0; i < TASK_LST_MAX_NUM; ++i)
		print_task(tasks, i);

	if (i > 0)
		putchar('\n');

	print_short_output(tasks);
	printf(" to do/done/total");
}

static int
read_crasfile(TaskLst *tasks, const char *crasfile)
{
	int read_stat;
	FILE *fp;

	fp = fopen(crasfile, "r");
	if (errno == ENOENT)
		return -1; /* We give the chance to create the file later. */

	if (fp == NULL)
		die("Could not read from %s: %s", crasfile, strerror(errno));

	read_stat = tasklst_read_from_file(tasks, fp);
	fclose(fp);

	if (read_stat < 0)
		die("Parsing error: task file corrupted.");

	if (tasklst_expired(*tasks) > 0)
		die("Due date passed (%d tasks overdue).", 
		    tasklst_tasks_todo(*tasks));

	return 0;
}

static void
write_crasfile(const char *crasfile, TaskLst tasks)
{
	FILE *fp;

	if ((fp = fopen(crasfile, "w")) == NULL)
		die("Could not write to %s: %s", crasfile, strerror(errno));

	tasklst_write_to_file(fp, tasks);
	fclose(fp);
}

static int
store_input(TaskLst *tasks, FILE *fp)
{
	char linebuf[TASK_LST_DESC_MAX_SIZE];
	int i;
	
	for (i = 0; i < TASK_LST_MAX_NUM; ++i) {
		fgets(linebuf, TASK_LST_DESC_MAX_SIZE, fp);
		if (feof(fp) != 0)
			return 0;

		linebuf[strlen(linebuf) - 1] = '\0'; /* Chomp '\n' */
		if (tasklst_add_task(tasks, TASK_TODO, linebuf) < 0)
			return -1;
	}

	return 0;
}

static void
usage(void)
{
	die("usage: cras [-nov] [-tT num] file");
}

static void
new_mode(const char *crasfile)
{
	int file_exists;
	TaskLst tasks;

	tasklst_init(&tasks);
	file_exists = read_crasfile(&tasks, crasfile);

	if (store_input(&tasks, stdin) < 0)
		fprintf(stderr, "Warning: Task file already full.\n");
	
	if (file_exists < 0) /* Only set if this is a new file */
		tasklst_set_expiration(&tasks, crasfile_expiry);
	write_crasfile(crasfile, tasks);
}

static void
output_mode(const char *crasfile, int mode)
{
	TaskLst tasks;

	tasklst_init(&tasks);
	read_crasfile(&tasks, crasfile);

	if (mode == SHORT_OUTPUT)
		print_short_output(tasks);
	else
		print_output(tasks);

	putchar('\n');
}

static void
mark_tasks_mode(const char *crasfile, const char *id, int value)
{
	int tasknum;
	char *endptr;
	TaskLst tasks;

	tasknum = strtol(id, &endptr, 10);
	if (endptr[0] != '\0')
		die("'%s' not a number.", id);

	tasklst_init(&tasks);
	read_crasfile(&tasks, crasfile);

	if (tasknum <= 0)
		die("Task number must be greater than zero.");

	if (tasknum <= tasklst_tasks_total(tasks))
		tasks.status[tasknum - 1] = value;
	else
		die("Task #%d does not exist.", tasknum);

	write_crasfile(crasfile, tasks);

	print_task(tasks, tasknum - 1);
}

int
main(int argc, char *argv[])
{
	char numarg[NUMARG_SIZE];
	int mode, task_value;

	mode = DEF_MODE;
	ARGBEGIN {
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
	case NEW_MODE:
		new_mode(argv[0]);
		return 0;
	case OUT_MODE:
		output_mode(argv[0], SHORT_OUTPUT);
		return 0;
	case MARK_MODE:
		mark_tasks_mode(argv[0], numarg, task_value);
		return 0;
	}
	
	/* Default behavior: long-form output */
	output_mode(argv[0], LONG_OUTPUT);

	return 0;
}
