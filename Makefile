# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = cras.c tasklst.c
OBJ = ${SRC:%.c=%.o}

all: options cras

options:
	@echo Build options:
	@echo "CFLAGS 	= ${CFLAGS}"
	@echo "LDFLAGS	= ${LDFLAGS}"
	@echo "CC	= ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

cras: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f cras ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f cras ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/cras

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/cras

.PHONY: all options clean install uninstall
