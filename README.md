# Cras - The Anti-Procrastination Tool

Cras is an unapologetic daily task planner and manager for your terminal and WM
 status bar. It holds your tasks only for a limited amount of time (24 hours, 
by default) and doesn't allow you to edit the task list after set up, except 
for marking a task as done.

## Build
Cras doesn't require any external dependencies.

Build by using:

```
$ make
```

## Usage

### Set up your task list
To start using Cras, you first need to set up your task list. This is done by 
using the -s command line option and entering a short description of your task 
in a new line. End your list hitting EOF (Ctrl+D) in a *blank line*.

```
$ cras -s
First task
Second task
Third task
```

You may also pipe in a text file if you so prefer.

```
$ cras -s < mytasklist
```

### Printing out your current list
To print out your current list, you may use either of two options: a long, 
detailed output, and a short summary (ideal for status bars). The long-form 
output is read just by running Cras without any further options:

```
$ cras
Tasks due for: Sat Jun 20 15:57:28 2020

#01 [TODO] Write README.md
#02 [TODO] Set up git repo for Cras
#03 [TODO] Succeed in life

3/0/3 to do/done/total
```

The short-form output is shown by using the -o option:

```
$ cras -o
3/0/3 to do/done/total
```

### Marking a task as done
When you've completed a task, use -t and the task number (as shown by the 
long-form output) to mark it as done.

```
$ cras -t 2
$ cras
Tasks due for: Sat Jun 20 15:57:28 2020

#01 [TODO] Write README.md
#02 [DONE] Set up git repo for Cras
#03 [TODO] Succeed in life

2/1/3 to do/done/total
```

If you need to mark a task again as pending, use -T.

## Install
You may install Cras by running the following command as root:

```
# make install
```

This will install the binary under $PREFIX/bin, as defined by your environment,
 or /usr/local/bin by default. The Makefile supports the $DESTDIR variable as 
well.

## License
Cras is licensed under the Apache Public License version 2.0. See LICENSE
 file for copyright and license details.
