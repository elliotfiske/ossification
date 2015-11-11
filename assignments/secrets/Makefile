# Makefile for the hello driver.
PROG=	secret_driver
SRCS=	secrets.c

DPADD+=	${LIBDRIVER} ${LIBSYS}
LDADD+=	-ldriver -lsys

MAN=

BINDIR?= /usr/sbin

.include <bsd.prog.mk>
