# cras - The Anti-Procrastination Tool

cras is an unapologetic daily task planner and manager for your terminal and WM
status bar. It holds your tasks only for a limited amount of time (24 hours, by
default).

## Build
cras doesn't require any external dependencies.

Build by using:

```
$ make
```

Customize the build process by changing config.mk to suit your needs.

User configuration is performed by modifying config.h. A set of defaults is 
provided in config.def.h.

## Usage
cras reads a task list from a file that is passed as an argument through the 
command line. Alternatively, you may export the ```CRAS_DEF_FILE``` variable
to set a default file to operate with. 

With no further options added, the default behavior is to output the pending 
tasks, but only if the expiration time has not passed yet.

```
$ cras my-dev-todo
Due date: Sat Jun 20 15:57:28 2020

#01 [TODO] Write README.md
#02 [TODO] Set up git repo for cras
#03 [TODO] Succeed in life

3/0 to do/done
```

To set a task list, pass the -n option and the name of the file that will hold
the list. The tasks will be read from standard input, each line being a new 
task. cras stops reading when it reaches EOF.
 
The -t and -T, followed by the task number, mark the task as done or pending, 
respectively.

Adding new tasks to an already existing file is possible by using the -a 
option. Tasks can be delated by using the -d option, specifying the number of 
the desired task. Editing the description of a task can be done by using -e.

For further usage information, please refer to the cras(1) manual page.

## Install
You may install cras by running the following command as root:

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
cras is published under an MIT/X11/Expat-type License. See LICENSE file for 
copyright and license details.
