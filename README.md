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

Customize the build process by changing config.mk to suit your needs.

User configuration is performed by modifying config.h. A set of defaults is 
provided in config.def.h.

## Usage

### Set up your task list
To start using Cras, you first need to set up your task list. This is done by 
using the -s command line option and a file name. Then enter a short 
description of your task in a new line. End your list hitting EOF (Ctrl+D) in a
 *blank line*.

```
$ cras -s mytask
First task
Second task
Third task
```

You may also pipe in a text file if you so prefer.

```
$ cras -s todo-today < mytasklist
```

### Printing out your current list
To print out your current list, you may use either of two options: a long, 
detailed output, and a short summary (ideal for status bars). The long-form 
output is read just by running Cras on your file without any further options:

```
$ cras my-dev-todo
Tasks due for: Sat Jun 20 15:57:28 2020

#01 [TODO] Write README.md
#02 [TODO] Set up git repo for Cras
#03 [TODO] Succeed in life

3/0/3 to do/done/total
```

The short-form output is shown by using the -o option:

```
$ cras -o my-dev-todo
3/0/3 to do/done/total
```

#### Deactivate colors in output
By default, the long-form output makes use of ANSI escapes to output in color. 
This can lead to corrupted output if your terminal doesn't support ANSI escapes
 or you're redirecting output to somewhere else. Cras supports ```NO_COLOR``` 
to switch off color in output:

```
$ NO_COLOR=1 cras my-dev-todo
```

### Marking a task as done
When you've completed a task, use -t and the task number (as shown by the 
long-form output) to mark it as done.

```
$ cras -t 2 my-dev-todo
$ cras my-dev-todo
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

## Cultural trivia
_cras_ means 'tomorrow' in Latin, hence the English word _procrastination_ 
means, literally, 'the act of posponing things for tomorrow.'

## License
Cras is licensed under the Apache Public License version 2.0. See LICENSE
 file for copyright and license details.
