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
	SET_MODE,
	OUT_MODE,
	MARK_MODE
};

static void die(const char *fmt, ...);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(TaskLst tasks, int i, int color);
static void print_short_output(TaskLst tasks);
static void print_output(TaskLst tasks, int color);
static void read_crasfile(TaskLst *tasks, const char *crasfile);
static void write_crasfile(const char *crasfile, TaskLst tasks);
static void read_user_input(TaskLst *tasks, FILE *fp);

static void usage(void);
static void set_tasks_mode(const char *crasfile);
static void output_mode(const char *crasfile, int mode, int color);
static void mark_tasks_mode(const char *crasfile, const char *id, int value, 
                            int color);

static void die(const char *fmt, ...)
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
print_task(TaskLst tasks, int i, int color)
{
	/* Null tasks are never printed */
	if (tasks.status[i] == TASK_VOID)
		return;

	printf("#%02d ", i + 1);
	if (tasks.status[i] == TASK_TODO)
		printf_color(((color > 0) ? task_todo_color : NULL), 
		             "%s ", task_todo_str);
	else /* TASK_DONE */
		printf_color(((color > 0) ? task_done_color : NULL), 
		             "%s ", task_done_str);

	printf("%s\n", tasks.tdesc[i]);
}


static void
print_short_output(TaskLst tasks)
{
	printf("%d/%d/%d to do/done/total\n", tasklst_tasks_todo(tasks),
	       tasklst_tasks_done(tasks), tasklst_tasks_total(tasks));
}

static void
print_output(TaskLst tasks, int color)
{
	int i;

	printf("Tasks due for: %s\n", ctime(&tasks.expiry));

	for(i = 0; i < TASK_LST_MAX_NUM; ++i)
		print_task(tasks, i, color);

	if (i > 0)
		putchar('\n');

	print_short_output(tasks);
}

static void
read_crasfile(TaskLst *tasks, const char *crasfile)
{
	int read_stat;
	FILE *fp;

	if ((fp = fopen(crasfile, "r")) == NULL)
		die("Could not read from %s: %s", crasfile, strerror(errno));

	read_stat = tasklst_read_from_file(tasks, fp);
	fclose(fp);

	if (read_stat < 0)
		die("Parsing error: task file corrupted.");

	if (tasklst_expired(*tasks) > 0)
		die("Current task file expired: Set a new one!");
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

static void
read_user_input(TaskLst *tasks, FILE *fp)
{
	char linebuf[TASK_LST_DESC_MAX_SIZE];
	int i;
	
	for (i = 0; i < TASK_LST_MAX_NUM; ++i) {
		fgets(linebuf, TASK_LST_DESC_MAX_SIZE, fp);
		if (feof(fp) != 0)
			break;
		
		linebuf[strlen(linebuf) - 1] = '\0';
		strncpy(tasks->tdesc[i], linebuf, TASK_LST_DESC_MAX_SIZE);
		tasks->status[i] = TASK_TODO;
	}
}

static void
usage(void)
{
	die("usage: cras [-osv] [-tT num] file");
}

static void
set_tasks_mode(const char *crasfile)
{
	TaskLst tasks;

	tasklst_init(&tasks);
	read_user_input(&tasks, stdin); /* Only stdin for now */
	
	tasklst_set_expiration(&tasks, crasfile_expiry);
	write_crasfile(crasfile, tasks);
}

static void
output_mode(const char *crasfile, int mode, int color)
{
	TaskLst tasks;

	tasklst_init(&tasks);
	read_crasfile(&tasks, crasfile);

	if (mode == SHORT_OUTPUT)
		print_short_output(tasks);
	else
		print_output(tasks, color);
}

static void
mark_tasks_mode(const char *crasfile, const char *id, int value, int color)
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

	print_task(tasks, tasknum - 1, color);
}

int
main(int argc, char *argv[])
{
	char numarg[NUMARG_SIZE];
	int color, mode, task_value;

	/* NO_COLOR support (https://no-color.org/) */
	if (getenv("NO_COLOR") != NULL)
		color = 0;
	else
		color = 1;

	mode = DEF_MODE;
	ARGBEGIN {
	case 's':
		if (mode != DEF_MODE)
			usage();
		mode = SET_MODE;
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
	case SET_MODE:
		set_tasks_mode(argv[0]);
		return 0;
	case OUT_MODE:
		output_mode(argv[0], SHORT_OUTPUT, color);
		return 0;
	case MARK_MODE:
		mark_tasks_mode(argv[0], numarg, task_value, color);
		return 0;
	}
	
	/* Default behavior: long-form output */
	output_mode(argv[0], LONG_OUTPUT, color);
	return 0;
}
