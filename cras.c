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
#include "tasklst.h"

enum {
	SHORT_OUTPUT,
	LONG_OUTPUT
};

static void die(const char *fmt, ...);
static void print_short_output(TaskLst tasks);
static void print_output(TaskLst tasks);
static void read_crasfile(TaskLst *tasks, const char *crasfile);
static void write_crasfile(const char *crasfile, TaskLst tasks);
static void read_user_input(TaskLst *tasks, FILE *fp);

static void usage(void);
static void set_tasks_mode(const char *crasfile);
static void output_mode(const char *crasfile, int mode);
static void mark_tasks_mode(const char *crasfile, const char *id, int mode);

/* 
 * crasfile_path defined here just for testing. It will be moved later to 
 * config.(def.)h 
 */
static char crasfile_path[] = "crasfile";

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
print_short_output(TaskLst tasks)
{
	printf("%d/%d/%d to do/done/total\n", tasklst_tasks_todo(tasks),
	       tasklst_tasks_done(tasks), tasklst_tasks_total(tasks));
}

static void
print_output(TaskLst tasks)
{
	int i;

	printf("Tasks due for: %s\n", ctime(&tasks.expiry));
	
	for(i = 0; i < TASK_LST_MAX_NUM; ++i) {
		if (tasks.status[i] != TASK_VOID)
			printf("#%02d [%s] %s\n", i + 1, 
			       (tasks.status[i] == TASK_TODO) ? "TODO" : "DONE",
			       tasks.tdesc[i]); 
	}

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
	die("[STUB: Usage info]");
}

static void
set_tasks_mode(const char *crasfile)
{
	TaskLst tasks;

	tasklst_init(&tasks);
	read_user_input(&tasks, stdin); /* Only stdin for now */
	
	tasklst_set_expiration(&tasks);
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
}

static void
mark_tasks_mode(const char *crasfile, const char *id, int mode)
{
	int tasknum;
	char *endptr;
	TaskLst tasks;

	tasknum = strtol(id, &endptr, 10);
	if (endptr[0] != '\0')
		die("%s not a number", id);

	tasklst_init(&tasks);
	read_crasfile(&tasks, crasfile);

	if (tasknum <= tasklst_tasks_total(tasks))
		tasks.status[tasknum - 1] = mode;
	else
		die("Task #%d does not exist.", tasknum);

	write_crasfile(crasfile, tasks);
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
		case 's':
			set_tasks_mode(crasfile_path);
			return 0;
		case 'o':
			output_mode(crasfile_path, SHORT_OUTPUT);
			return 0;
		case 't':
			mark_tasks_mode(crasfile_path, EARGF(usage()),
			                TASK_DONE);
			return 0;
		case 'T':
			mark_tasks_mode(crasfile_path, EARGF(usage()), 
			                TASK_TODO);
			return 0;
		default:
			usage(); /* usage() dies, so nothing else needed. */
	} ARGEND;

	/* Default behavior: long-form output */
	output_mode(crasfile_path, LONG_OUTPUT);
	return 0;
}
