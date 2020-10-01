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
Cras reads a task list from a file that is passed as an argument through the 
command line. With no further options added, the default behavior is to output
the pending tasks, but only if the expiration time has not passed yet.

```
$ cras my-dev-todo
Tasks due for: Sat Jun 20 15:57:28 2020

#01 [TODO] Write README.md
#02 [TODO] Set up git repo for Cras
#03 [TODO] Succeed in life

3/0/3 to do/done/total
```

To set a task list, pass the -n option and the name of the file that will hold
 the list. The tasks will be read from standard input, each line being a new 
task. Cras stops reading when it reaches EOF.
 
The -t and -T, followed by the task number, mark the task as done or pending, 
respectively.

Adding new tasks to an already existing file is possible by using the -a 
option. Deleting tasks is performed by using the -d option.

For further usage information, please refer to the cras(1) manual page.

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
