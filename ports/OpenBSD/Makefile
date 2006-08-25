# $Id: Makefile,v 1.1 2006-08-25 12:32:52 nicm Exp $

COMMENT=	"fetches and delivers mail"

DISTNAME=	fdm-0.1
CATEGORIES=	mail

MAINTAINER=     Nicholas Marriott <nicm@users.sourceforge.net>

PERMIT_PACKAGE_CDROM=	Yes
PERMIT_PACKAGE_FTP=	Yes
PERMIT_DISTFILES_CDROM=	Yes
PERMIT_DISTFILES_FTP=	Yes

WANTLIB= c crypto ssl

MASTER_SITES=	${MASTER_SITE_SOURCEFORGE:=fdm/}

post-install:
	${INSTALL_DATA_DIR} ${PREFIX}/share/doc/fdm
	${INSTALL_DATA} ${WRKSRC}/README ${PREFIX}/share/doc/fdm

.include <bsd.port.mk>
