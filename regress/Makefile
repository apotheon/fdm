# $Id: Makefile,v 1.16 2007-10-10 20:32:07 nicm Exp $

CLEANFILES= *~

TESTFILES!= echo *.test

TARGETS=
.for n in ${TESTFILES}
TARGETS+= ${n}
${n}:
		@echo ${n}:
		@HOME=. awk -v CMD="../fdm -vvvnf /dev/stdin" \
			-v DEBUG=${DEBUG} -f test.awk ${n}
.endfor

.MAIN:		all
.PHONY: 	regress clean ${TARGETS}
.PRECIOUS: 	${TARGETS}

all:		regress

regress:	${TARGETS}
		cd tests && ${MAKE}
		cd test-pop3 && ${MAKE}

clean:
		rm -f ${CLEANFILES}
