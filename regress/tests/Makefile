# $Id: Makefile,v 1.2 2008-12-17 22:36:23 nicm Exp $

CLEANFILES= *~ *.in *.out *.data *.conf

TESTFILES!= echo *.sh

TARGETS=
.for n in ${TESTFILES}
TARGETS+= ${n}
${n}:
		@HOME=. FDM="../../fdm -h" sh ${n}
.endfor

.MAIN:		all
.PHONY: 	regress clean ${TARGETS}
.PRECIOUS: 	${TARGETS}

all:		regress

regress:	${TARGETS}

clean:
		rm -f ${CLEANFILES}
