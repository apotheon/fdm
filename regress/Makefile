# $Id: Makefile,v 1.18 2008-12-17 22:36:23 nicm Exp $

CLEANFILES= *~

TESTFILES!= echo *.test

TARGETS=
.for n in ${TESTFILES}
TARGETS+= ${n}
${n}:
		@echo ${n}:
		@HOME=. awk -v CMD="../fdm -hvvvnf /dev/stdin" \
			-v DEBUG=${DEBUG} -f test.awk ${n}
.endfor

.MAIN:		all
.PHONY: 	regress clean ${TARGETS}
.PRECIOUS: 	${TARGETS}

all:		regress

regress:	${TARGETS}
		cd tests && ${MAKE}
		cd test-pop3 && ${MAKE}
		cd test-imap && ${MAKE}

clean:
		rm -f ${CLEANFILES}
