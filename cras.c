/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <sline.h> /* Depends on stddef.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "date.h"
#include "strlcpy.h"
#include "tasklst.h"

#define TASK_NONEXIST_MSG "Task #%d does not exist."
#define NUMARG_SIZE 5 /* 4 digits + '\0' for arg of -d/-e/-t/-T */

enum {
	APP_MODE,
	DEF_MODE,
	DLT_MODE,
	EDIT_MODE,
	MARK_MODE,
	NEW_MODE
};

/* Auxiliary functions */
static void die(const char *fmt, ...);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(Task task, int i);
static int print_task_list(TaskLst list);
static void read_file(TaskLst *list, const char *fname);
static void write_file(const char *fname, TaskLst list);
static int parse_tasknum(const char *id);
static void usage(void);

/* Execution modes */
static void delete_mode(const char *fname, const char *id);
static void edit_mode(const char *fname, const char *id);
static void input_mode(const char *fname, const char *date, int append);
static void mark_list_mode(const char *fname, const char *id, int value);
static void output_mode(const char *fname);

static int sline_mode; /* Are we using sline or not? */

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
	if (getenv("NO_COLOR") != NULL)
		eff_color = NULL;
	else
		eff_color = ansi_color;

	printf("%s", eff_color != NULL ? eff_color : "");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("%s", eff_color != NULL ? "\033[0m" : "");
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
read_file(TaskLst *list, const char *fname)
{
	int read_stat;
	FILE *fp;
	char date_buf[DATE_SIZE];

	fp = fopen(fname, "r");
	if (fp == NULL)
		die("Could not read %s: %s.", fname, strerror(errno));

	read_stat = task_lst_read_from_file(list, fp);
	fclose(fp);

	if (read_stat < 0) {
		task_lst_cleanup(list);
		die("%s: not a cras file.", fname);
	}

	if (task_lst_on_date(*list) < 0) {
		strlcpy(date_buf, list->date, DATE_SIZE);
		task_lst_cleanup(list);
		die("%s: valid on %s.", fname, date_buf);
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
	die("usage: cras [-anv] [-detT num] [-w date] [file]");
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
	Task *task;
	char newstr[TASK_LST_BUF_SIZE];

	tasknum = parse_tasknum(id);

	read_file(&list, fname);

	if ((task = task_lst_get_task(list, tasknum - 1)) == NULL) {
		task_lst_cleanup(&list);
		die(TASK_NONEXIST_MSG, tasknum);
	}

	if (sline_mode > 0) {
		sline_set_prompt("[Edit #%02d: %s]: ", tasknum, task->tdesc);
		if (sline(newstr, TASK_LST_BUF_SIZE) < 0) {
			if (sline_err != SLINE_ERR_EOF) {
				task_lst_cleanup(&list);
				die("sline: %s.\n", sline_errmsg());
			}
		}
	} else {
		fgets(newstr, TASK_LST_BUF_SIZE, stdin);
	}
	task_set_tdesc(task, newstr);

	write_file(fname, list);

	task_lst_cleanup(&list);
}

static void
input_mode(const char *fname, const char *date, int append)
{
	int task_num;
	TaskLst list;
	char linebuf[TASK_LST_BUF_SIZE];

	if (append > 0)
		read_file(&list, fname);
	else
		task_lst_init(&list);

	while (feof(stdin) == 0) {
		if (sline_mode > 0) {
			sline_set_prompt("#%02d: ", task_num + 1);
			if (sline(linebuf, TASK_LST_BUF_SIZE) < 0 
			    && sline_err != SLINE_ERR_EOF) {
				task_lst_cleanup(&list);
				die("sline: %s.", sline_errmsg());
			} else if (sline_err == SLINE_ERR_EOF) {
				break;
			}
		} else if (fgets(linebuf, TASK_LST_BUF_SIZE, stdin) == NULL) {
			break;
		}

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

		if (task_lst_add_task(&list, TASK_TODO, linebuf) < 0) {
			task_lst_cleanup(&list);
			die("Internal memory error.");
		}

		++task_num;
	}

	if (task_num == 0) {
		task_lst_cleanup(&list);
		die("Aborting: empty task list.");
	} else { /* DELETE branch */
		/* Only set a new date if creating a new file */
		if (append == 0)
			task_lst_set_date(&list, date);

		write_file(fname, list);

		task_lst_cleanup(&list);
	}
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
output_mode(const char *fname)
{
	TaskLst list;

	read_file(&list, fname);

	printf("%s\n", list.date);
	print_task_list(list);

	task_lst_cleanup(&list);
}

int
main(int argc, char *argv[])
{
	char *numarg, *datearg;
	const char *fileptr;
	int opt, mode, date, task_value;

	mode = DEF_MODE;
	numarg = NULL;
	datearg = NULL;
	date = 0;
	while ((opt = getopt(argc, argv, ":anvd:e:t:T:w:")) != -1) {
		switch (opt) {
		case 'a':
			mode = APP_MODE;
			break;
		case 'n':
			mode = NEW_MODE;
			break;
		case 'v':
			die("cras %s", VERSION);
			break;
		case 'd':
			mode = DLT_MODE;
			numarg = optarg;
			break;
		case 'e':
			mode = EDIT_MODE;
			numarg = optarg;
			break;
		case 't':
			mode = MARK_MODE;
			task_value = TASK_DONE;
			numarg = optarg;
			break;
		case 'T':
			mode = MARK_MODE;
			task_value = TASK_TODO;
			numarg = optarg;
			break;
		case 'w':
			date = 1;
			datearg = optarg;
			break;
		default:
			usage(); /* usage() dies, so nothing else needed. */
		}
	}

	if (optind >= argc) {
		if ((fileptr = getenv("CRAS_DEF_FILE")) == NULL)
			die("CRAS_DEF_FILE environment variable not set.");
	} else {
		fileptr = argv[optind];
	}

	if (datearg != NULL && is_date(datearg) < 0)
		die("Invalid date format.");

	if ((sline_mode = isatty(STDIN_FILENO)) > 0) {
		if (sline_setup(0) < 0) /* Set up sline without history */
			die("sline: %s.", sline_errmsg());
		atexit(sline_end);
	}

	switch (mode) {
	case APP_MODE:
		input_mode(fileptr, NULL, 1);
		return 0;
	case DLT_MODE:
		delete_mode(fileptr, numarg);
		return 0;
	case EDIT_MODE:
		edit_mode(fileptr, numarg);
		return 0;
	case MARK_MODE:
		mark_list_mode(fileptr, numarg, task_value);
		return 0;
	case NEW_MODE:
		input_mode(fileptr, date > 0 ? datearg : NULL, 0);
		return 0;
	default:
		output_mode(fileptr);
		return 0;
	}

	return 0;
}
