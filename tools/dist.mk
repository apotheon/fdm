# $Id: dist.mk,v 1.2 2009-05-22 10:58:14 nicm Exp $

VERSION= 1.6

DISTDIR= fdm-${VERSION}
DISTFILES= *.[chl] Makefile GNUmakefile configure *.[1-9] fdm-sanitize \
	   README MANUAL TODO CHANGES \
	   `find examples compat regress -type f -and ! -path '*CVS*'`

dist:          	manual
		(./configure &&	make clean-all)
		grep '^#FDEBUG=' Makefile
		grep '^#FDEBUG=' GNUmakefile
		[ "`(grep '^VERSION' Makefile; grep '^VERSION' GNUmakefile)| \
		        uniq -u`" = "" ]
		chmod +x configure
		tar -zc \
		        -s '/.*/${DISTDIR}\/\0/' \
		        -f ${DISTDIR}.tar.gz ${DISTFILES}

manual:
		awk -f tools/makemanual.awk MANUAL.in > MANUAL

yannotate:
		awk -f tools/yannotate.awk parse.y > parse.y.new
		mv parse.y.new parse.y
		trim parse.y

upload-index.html: update-index.html
		scp index.html nicm,fdm@web.sf.net:/home/groups/f/fd/fdm/htdoc

update-index.html: manual
		nroff -mdoc fdm.conf.5|m2h -u > fdm.conf.5.html
		nroff -mdoc fdm.1|m2h -u > fdm.1.html
		awk -v V=${VERSION} -f tools/makeindex.awk \
			index.html.in > index.html
		rm -f fdm.conf.5.html fdm.1.html
