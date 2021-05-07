/* See LICENSE file for copyright and license details. */

/* 
 * task_todo_color, task_done_color: colors in which task_todo_str and 
 * task_done_str, respectively, are to be printed in. Colors must be set using 
 * ANSI escape codes.
 */
static const char task_todo_color[] = "\033[31;1m"; /* Red */
static const char task_done_color[] = "\033[32;1m"; /* Green */

/* 
 * task_todo_str, task_done_str: the appearance of the status labels can be 
 * modified here.
 */
static const char task_todo_str[] = "[TODO]";
static const char task_done_str[] = "[DONE]";

