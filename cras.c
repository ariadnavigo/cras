/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <sline.h>
#include <stdarg.h> /* Dependency for strlcpy.h */
#include <stddef.h>
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
static void cleanup(void);
static void usage(void);
static int parse_tasknum(const char *id);

/* I/O */
static int fd_input(char *linebuf);
static int prompt_input(char *linebuf, const char *initstr);
static void printf_color(const char *ansi_color, const char *fmt, ...);
static void print_task(Task task, int i);
static int print_task_list(void);
static void read_file(const char *fname, int allow_future);
static void write_file(const char *fname);

/* Execution modes */
static void delete_mode(const char *fname, const char *id);
static void edit_mode(const char *fname, const char *id);
static void input_mode(const char *fname, const char *date, int append);
static void mark_list_mode(const char *fname, const char *id, int value);
static void output_mode(const char *fname);

static TaskLst list;
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
cleanup(void)
{
	task_lst_cleanup(&list);

	if (sline_mode > 0)
		sline_end();
}

static void
usage(void)
{
	die("usage: cras [-anv] [-detT num] [-w date] [file]");
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

static int
fd_input(char *linebuf)
{
	static int line = 1;

	int trash;
	char *last_chr;

	if (fgets(linebuf, TASK_TDESC_SIZE, stdin) == NULL)
		return -1;

	last_chr = &linebuf[strlen(linebuf) - 1];

	/* Flushing stdin if there's more input; chomping '\n' if not. */
	if (*last_chr != '\n') {
		fprintf(stderr, "Warn: stdin: line %d truncated (too long).\n",
		        line);
		while ((trash = fgetc(stdin)) != '\n' && trash != EOF);
	} else {
		*last_chr = '\0';
	}

	++line;

	return 0;
}

static int
prompt_input(char *linebuf, const char *initstr)
{
	int sline_ret;

	sline_ret = sline(linebuf, TASK_TDESC_SIZE, initstr);
	if (sline_ret < 0 && sline_err != SLINE_ERR_EOF)
		die("sline: %s.", sline_errmsg());
	else if (sline_err == SLINE_ERR_EOF)
		return -1;

	return 0;
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
		printf_color(task_todo_color, "%s", task_todo_str);
	else /* TASK_DONE */
		printf_color(task_done_color, "%s", task_done_str);

	printf(" %s\n", task.tdesc);
}

static int
print_task_list(void)
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
read_file(const char *fname, int allow_future)
{
	int read_stat, date_res;
	FILE *fp;
	char date_buf[DATE_SIZE];

	fp = fopen(fname, "r");
	if (fp == NULL)
		die("Could not read %s: %s.", fname, strerror(errno));

	read_stat = task_lst_read_from_file(&list, fp);
	fclose(fp);

	if (read_stat < 0)
		die("%s: not a cras file.", fname);

	date_res = date_cmp(list.date);
	if (date_res < 0 || (allow_future == 0 && date_res > 0)) {
		strlcpy(date_buf, list.date, DATE_SIZE);
		die("%s: passed deadline (was: %s).", fname, date_buf);
	}
}

static void
write_file(const char *fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "w")) == NULL)
		die("Could not write to %s: %s", fname, strerror(errno));

	task_lst_write_to_file(fp, list);
	fclose(fp);
}

static void
delete_mode(const char *fname, const char *id)
{
	int tasknum;

	tasknum = parse_tasknum(id);

	read_file(fname, 1);

	if (task_lst_del_task(&list, tasknum - 1) < 0)
		die(TASK_NONEXIST_MSG, tasknum);

	write_file(fname);
	fprintf(stderr, "Task #%d deleted.\n", tasknum);
}

static void
edit_mode(const char *fname, const char *id)
{
	int tasknum, input_stat;
	Task *task;
	char newstr[TASK_TDESC_SIZE];

	tasknum = parse_tasknum(id);

	read_file(fname, 1);

	if ((task = task_lst_get_task(list, tasknum - 1)) == NULL)
		die(TASK_NONEXIST_MSG, tasknum);

	input_stat = 0;
	if (sline_mode > 0) {
		sline_set_prompt("#%02d: ", tasknum);
		input_stat = prompt_input(newstr, task->tdesc);
	} else if (fd_input(newstr) < 0) {
		input_stat = -1;
	}

	if (input_stat < 0)
		return;

	if (strlen(newstr) == 0)
		return;

	task_set_tdesc(task, newstr);

	write_file(fname);
	fprintf(stderr, "Task #%d edited.\n", tasknum);
}

static void
input_mode(const char *fname, const char *date, int append)
{
	int tasknum, input_stat;
	char linebuf[TASK_TDESC_SIZE];

	if (append > 0)
		read_file(fname, 1);
	else
		task_lst_init(&list);

	tasknum = task_lst_get_size(list);
	input_stat = 0;
	while (feof(stdin) == 0) {
		if (sline_mode > 0) {
			sline_set_prompt("#%02d: ", tasknum + 1);
			input_stat = prompt_input(linebuf, NULL);
		} else if (fd_input(linebuf) < 0) {
			input_stat = -1;
		}

		if (input_stat < 0)
			break;

		if (linebuf[strlen(linebuf) - 1] == '\n')
			linebuf[strlen(linebuf) - 1] = '\0';

		if (strlen(linebuf) == 0)
			continue;

		if (task_lst_add_task(&list, TASK_TODO, linebuf) < 0)
			die("Internal memory error.");

		++tasknum;
	}

	if (tasknum == 0)
		die("Aborting: empty task list.");

	/* Only set a new date if creating a new file */
	if (append == 0)
		task_lst_set_date(&list, date);

	write_file(fname);
	fprintf(stderr, "Task list saved.\n");
}

static void
mark_list_mode(const char *fname, const char *id, int value)
{
	int tasknum;
	Task *task;

	tasknum = parse_tasknum(id);

	read_file(fname, 0);

	task = task_lst_get_task(list, tasknum - 1);
	if (task == NULL)
		die(TASK_NONEXIST_MSG, tasknum);

	task->status = value;
	write_file(fname);

	print_task(*task, tasknum - 1);
}

static void
output_mode(const char *fname)
{
	read_file(fname, 1);

	printf("%s\n", list.date);
	print_task_list();
}

int
main(int argc, char *argv[])
{
	char *numarg, *datearg;
	const char *fileptr;
	int opt, mode, date, task_value;

	atexit(cleanup);

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
			printf("cras %s (sline %s)", VERSION, sline_version());
			putchar('\n'); /* Don't wanna clutter the line above */
			return 0;
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
			usage();
			break;
		}
	}

	if (optind >= argc) {
		if ((fileptr = getenv("CRAS_DEF_FILE")) == NULL)
			die("CRAS_DEF_FILE environment variable not set.");
	} else {
		fileptr = argv[optind];
	}

	if (datearg != NULL) {
		if (is_date(datearg) < 0)
			die("Invalid date format.");

		if (date_cmp(datearg) < 0)
			die("Date is in the past.");
	}

	if ((sline_mode = isatty(STDIN_FILENO)) > 0) {
		if (sline_setup(0) < 0) /* Set up sline without history */
			die("sline: %s.", sline_errmsg());
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
