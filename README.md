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

Customize the build process by changing ``config.mk`` to suit your needs.

User configuration is performed by modifying ``config.h``. A set of defaults is 
provided in ``config.def.h``.

## Install
You may install cras by running the following command as root:

```
# make install
```

This will install the binary under ``$PREFIX/bin``, as defined by your 
environment, or ``/usr/local/bin`` by default. The Makefile supports the 
``$DESTDIR`` variable as well.

## Cultural trivia
_cras_ means 'tomorrow' in Latin, hence the English word _procrastination_ 
means, literally, 'the act of posponing things for tomorrow.'

## Contributing

All contributions are welcome! If you wish to send in patches, ideas, or report
a bug, you may do so by sending an email to the 
[cras-devel](https://lists.sr.ht/~arivigo/cras-devel) mailing list.

If interested in getting some news from the project, you may also want to 
subscribe to the low-volume 
[cras-announce](https://lists.sr.ht/~arivigo/cras-announce) mailing list!

## License
cras is published under an MIT/X11/Expat-type License. See ``LICENSE`` file for 
copyright and license details.