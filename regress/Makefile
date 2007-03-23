# $Id: Makefile,v 1.12 2007-03-23 16:45:10 nicm Exp $

CLEANFILES= *~

TESTFILES= account.test macros.test options.test proxy.test nested.test \
	   size.test attachment.test

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

clean:
		rm -f ${CLEANFILES}
