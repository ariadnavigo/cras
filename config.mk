# Cras version
VERSION = 0.1.0

# Customize below to your needs

# Paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man

# Flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
#CFLAGS = -g -std=c99 -Wpedantic -Wall -Wextra ${CPPFLAGS}
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra ${CPPFLAGS}
#LDFLAGS = -static

# Compiler and linker
CC = cc

