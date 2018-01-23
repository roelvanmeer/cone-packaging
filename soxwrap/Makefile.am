#$Id: Makefile.am,v 1.10 2004/12/05 02:24:29 mrsam Exp $
#
# Copyright 2000-2004 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

DISTCLEANFILES=soxlibs.dep

noinst_LIBRARIES=libsoxwrap.a libmksocket.a

libsoxwrap_a_SOURCES=soxwrap.h \
	sconnect.c sconnect.h

libmksocket_a_SOURCES=mksocket.c mksocket.h

noinst_PROGRAMS=testprog

testprog_SOURCES=testprog.c
testprog_DEPENDENCIES=libsoxwrap.a config.status
testprog_LDADD=libsoxwrap.a @SOCKLIBS@

check-am:
	./testprog
