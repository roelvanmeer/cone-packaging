# $Id: Makefile.am,v 1.5 2007/06/30 15:40:53 mrsam Exp $
#
# Copyright 2001-2002 Double Precision, Inc.  See COPYING for
# distribution information.


EXTRA_DIST=certs.sh mkcerts.pl

noinst_DATA=rootcerts.cnf

clean-local:
	rm -rf rootcerts.cnf certs

DISTCLEANFILES=rootcertsdir.cnf

if HAVE_OPENSSL
rootcerts.cnf: config.status certs.sh
	rm -rf certs
	mkdir certs
	@SHELL@ $(srcdir)/certs.sh
	@REHASH@ certs >/dev/null
	echo 'files="'`ls certs`'"' >rootcerts.cnf

install-data-hook: rootcerts.cnf
	rm -rf $(DESTDIR)@certdb@
	-$(mkinstalldirs) $(DESTDIR)@certdb@
	chmod 755 $(DESTDIR)@certdb@
	ls certs | @PERL@ -ne ' chomp; if ( -l "certs/$$_" ) { print "$(LN_S) " . readlink("certs/$$_") . " $(DESTDIR)@certdb@/$$_\n"; } else { print "$(INSTALL_DATA) certs/$$_ $(DESTDIR)@certdb@\n"; }' | @SHELL@

install-perms:
	echo @certdb@ 555 bin bin >permissions.dat
	ls certs | sed 's:^:@certdb@/:;s:$$: 444 bin bin:' >>permissions.dat

uninstall-local: rootcerts.cnf
	. rootcerts.cnf ; cd $(DESTDIR)@certdb@ && rm -f $$files

else
rootcerts.cnf:
	echo 'files=""' >rootcerts.cnf

install-data-hook:
	:

install-perms:
	cp /dev/null permissions.dat

uninstall-local:
	:
endif

