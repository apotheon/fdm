# $Id: Makefile,v 1.2 2007-08-15 23:44:47 nicm Exp $

CLEANFILES= *~ *.in *.out *.data *.conf

TESTFILES!= echo *.sh

TARGETS=
.for n in ${TESTFILES}
TARGETS+= ${n}
${n}:
		@HOME=. FDM=../../fdm sh ${n}
.endfor

.MAIN:		all
.PHONY: 	regress clean ${TARGETS}
.PRECIOUS: 	${TARGETS}

all:		regress

regress:	${TARGETS}

clean:
		rm -f ${CLEANFILES}
