# $Id: GNUmakefile,v 1.17 2006-11-16 23:34:25 nicm Exp $

.PHONY: clean

PROG = fdm
VERSION = 0.6
DATE=$(shell date +%Y%m%d-%H%M)

## Installation parameters

PREFIX = /usr/local

BIN_OWNER = bin
BIN_GROUP = root

### Programs

CC = gcc

ifeq ($(shell uname),SunOS)
YACC = yacc
YFLAGS = -d
else
YACC = bison
YFLAGS = -dy
endif

LEX = flex
LFLAGS = -l

INSTALLBIN = install -D -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLMAN = install -D -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

### Compilation

SRCS= fdm.c log.c xmalloc.c io.c replace.c connect.c mail.c \
      fetch-pop3.c fetch-imap.c fetch-stdin.c deliver-smtp.c deliver-pipe.c \
      deliver-drop.c deliver-maildir.c deliver-mbox.c deliver-write.c \
      deliver-append.c deliver-rewrite.c match-regexp.c child.c parent.c \
      privsep.c \
      y.tab.c lex.yy.c

DEFS = $(shell getconf LFS_CFLAGS) -DBUILD="\"$(VERSION) ($(DATE))\""

ifeq ($(shell uname),Linux)
SRCS += strlcpy.c strlcat.c strtonum.c
DEFS += -D_GNU_SOURCE -DNO_STRLCPY -DNO_STRLCAT -DNO_SETPROCTITLE -DNO_STRTONUM
endif

OBJS = $(patsubst %.c,%.o,$(SRCS))
CPPFLAGS = $(DEFS) -I.
CFLAGS+= -std=c99 -pedantic -Wno-long-long -Wall -W -Wnested-externs \
	-Wformat=2 -Wmissing-prototypes -Wstrict-prototypes \
	-Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual \
	-Wsign-compare -Wredundant-decls

LIBS = -lssl

CLEANFILES = $(PROG) y.tab.c lex.yy.c y.tab.h $(OBJS) .depend

all: fdm

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+

depend: $(SRCS)
	$(CC) -MM $(SRCS) > .depend

y.tab.c y.tab.h: parse.y
	$(YACC) $(YFLAGS) $<

lex.yy.c: lex.l
	$(LEX) $(LFLAGS) $<

install:
	$(INSTALLBIN) $(PROG) $(PREFIX)/sbin/$(PROG)
	$(INSTALLMAN) $(PROG).1 $(PREFIX)/man/man1/$(PROG).1
	$(INSTALLMAN) $(PROG).conf.5 $(PREFIX)/man/man5/$(PROG).conf.5

clean:
	rm -f $(CLEANFILES)

ifeq ($(wildcard .depend),.depend)
include .depend
endif
