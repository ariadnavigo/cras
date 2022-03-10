# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = cras.c date.c strlcpy.c tasklst.c
OBJ = ${SRC:.c=.o}

all: options cras

options:
	@echo Build options:
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

cras: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS} ${LIBS}

clean:
	rm -f cras ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f cras ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/cras
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" cras.1 \
	    > ${DESTDIR}${MANPREFIX}/man1/cras.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/cras.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/cras ${DESTDIR}${MANPREFIX}/man1/cras.1

.PHONY: all options clean install uninstall
