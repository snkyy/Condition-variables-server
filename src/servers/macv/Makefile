# Makefile for MACV server
PROG=	macv
SRCS=	main.c

DPADD+=	${LIBSYS}
LDADD+=	-lsys -lminlib -ltimers -lcompat_minix

MAN=

BINDIR?= /usr/sbin
FILES=macv.conf
FILESNAME=macv
FILESDIR= /etc/system.conf.d

.include <minix.service.mk>
