#
# Copyright 1998 - 2011 Double Precision, Inc.  See COPYING for
# distribution information.

noinst_LTLIBRARIES=librfc2045.la
noinst_PROGRAMS=reformime makemime headercheck testrfc3676parser

BUILT_SOURCES=reformime.html reformime.1 makemime.html makemime.1 \
	rfc2045.html rfc2045.3

EXTRA_DIST=$(BUILT_SOURCES) testsuite testsuite.txt.idn testsuite.txt.noidn \
	testsuitemm testsuitemm.txt \
	testrfc3676parsersuite testrfc3676parsersuite.txt

noinst_DATA=$(BUILT_SOURCES)

librfc2045_la_SOURCES=rfc2045.c rfc2045.h rfc2045src.h \
		     rfc2045acchk.c rfc2045acprep.c \
		     rfc2045appendurl.c rfc2045cdecode.c rfc2045decode.c \
		     rfc2045enomem.c \
		     rfc2045_fromfd.c \
                     rfc2045find.c rfc2045mkboundary.c rfc2045rewrite.c \
                     rfc2045tryboundary.c rfc2045xdump.c \
		     rfc2045searchcontenttype.c rfc2045decodemimesection.c \
		     rfc2045decodemimesectionu.c rfc2045header.c \
		     rfc2045reply.c \
		     rfc2045decodemsgtoutf8.c \
		     rfc2231.c rfc2231encode.c \
		     rfc3676parser.h rfc3676parser.c rfc3676parsercpp.C \
		     base64.c base64.h

reformime_SOURCES=reformime.c
reformime_LDADD = librfc2045.la ../rfc822/libencode.la ../rfc822/librfc822.la \
		../unicode/libunicode.la ../numlib/libnumlib.la
reformime_DEPENDENCIES = $(reformime_LDADD)
reformime_LDFLAGS=-static

makemime_SOURCES=makemime.c
makemime_LDADD = ../rfc822/libencode.la ../rfc822/librfc822.la librfc2045.la ../unicode/libunicode.la ../numlib/libnumlib.la ../unicode/libunicode.la
makemime_DEPENDENCIES=$(makemime_LDADD)
makemime_LDFLAGS=-static

headercheck_SOURCES=headercheck.c
headercheck_LDADD=librfc2045.la ../rfc822/librfc822.la ../numlib/libnumlib.la \
		../unicode/libunicode.la
headercheck_DEPENDENCIES=$(headercheck_LDADD)
headercheck_LDFLAGS=-static

testrfc3676parser_SOURCES=testrfc3676parser.c
testrfc3676parser_LDADD=librfc2045.la ../unicode/libunicode.la
testrfc3676parser_DEPENDENCIES=$(testrfc3676parser_LDADD)

if HAVE_SGML
reformime.html: reformime.sgml ../docbook/sgml2html
	../docbook/sgml2html reformime.sgml reformime.html

reformime.1: reformime.sgml ../docbook/sgml2html
	../docbook/sgml2man reformime.sgml reformime.1

makemime.html: makemime.sgml ../docbook/sgml2html
	../docbook/sgml2html makemime.sgml makemime.html

makemime.1: makemime.sgml ../docbook/sgml2html
	../docbook/sgml2man makemime.sgml makemime.1

rfc2045.html: rfc2045.sgml ../docbook/sgml2html
	../docbook/sgml2html rfc2045.sgml rfc2045.html

rfc2045.3: rfc2045.sgml ../docbook/sgml2html
	../docbook/sgml2man rfc2045.sgml rfc2045.3
endif

if HAVE_LIBIDN
TESTSUITE=testsuite.txt.idn
else
TESTSUITE=testsuite.txt.noidn
endif

check-am:
	@SHELL@ $(srcdir)/testsuite | cmp -s - $(srcdir)/$(TESTSUITE)
	@SHELL@ $(srcdir)/testsuitemm | cmp -s - $(srcdir)/testsuitemm.txt
	@SHELL@ $(srcdir)/testrfc3676parsersuite | diff -U 3 $(srcdir)/testrfc3676parsersuite.txt -
