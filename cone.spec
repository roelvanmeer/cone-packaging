# $Id: cone.spec.in,v 1.12 2006/02/17 01:20:52 mrsam Exp $

# Custom build against libcurses-5.3

%define custom_curses %(test -d /usr/include/ncursesw >/dev/null 2>&1 && echo 0 && exit 0; echo 1)

%if %custom_curses
%define curses_include_dir  /usr/include/ncurses53w
%define curses_lib_dir /usr/lib/ncurses53

BuildRequires: %{curses_include_dir}
BuildRequires: %{curses_lib_dir}

%else
BuildRequires: ncurses-devel

%define curses_include_dir  /usr/include/ncursesw

%endif

%define is_not_mandrake %(test ! -e /etc/mandrake-release && echo 1 || echo 0)

%define import_rootcerts %(rpm -q courier >/dev/null 2>&1 && echo 1 || echo 0)

%if %is_not_mandrake
%define cone_release %(release="`rpm -q --queryformat='.%{VERSION}' redhat-release 2>/dev/null`" ; if test $? != 0 ; then release="`rpm -q --queryformat='.%{VERSION}' fedora-release 2>/dev/null`" ; if test $? != 0 ; then release="" ; fi ; fi ; echo "$release")
%else
%define cone_release mdk
%endif

Summary: CONE mail reader
Name: cone
Version: 0.69
Release: 1%{cone_release}
URL: http://www.courier-mta.org/cone
Source0: %{name}-%{version}.tar.bz2
License: GPL
Group: Applications/Internet
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot


BuildRequires: aspell-devel libxml2-devel
BuildRequires: zlib-devel openssl-devel /usr/include/fam.h perl
BuildRequires: libstdc++-devel gcc-c++
BuildRequires: openldap-devel

%if %is_not_mandrake
BuildRequires: openssl-perl
%endif

%if %import_rootcerts
Requires: %(eval `courier-config` ; echo $datadir/rootcerts )
%endif

Requires(post): %{__perl}

%description
CONE is a simple, text-based E-mail reader and writer.

%package devel
Group: Development/Libraries
Summary: LibMAIL mail client development library.
Requires: %{name} = 0:%{version}-%{release}


%description devel
The %{name}-devel package the header files and library files for developing
application using LibMAIL - a high level, C++ OO library for mail clients.

%prep
%setup -q

CPPFLAGS="$CPPFLAGS -I %{curses_include_dir}"
export CPPFLAGS
%if %custom_curses
LDFLAGS="$LDFLAGS -L %{curses_lib_dir}"
export LDFLAGS
%endif

%configure -C --with-devel
%build
%{__make} %{?_smp_mflags}
%install
rm -rf $RPM_BUILD_ROOT
%{__make} install DESTDIR=$RPM_BUILD_ROOT
%{__install} sysconftool $RPM_BUILD_ROOT%{_datadir}/cone/cone.sysconftool
touch $RPM_BUILD_ROOT%{_sysconfdir}/cone

# Remove dupe copies of doc/html from the install tree.

ls cone/html | ( cd $RPM_BUILD_ROOT%{_datadir}/cone && xargs -n10 rm -f )

%clean
rm -rf $RPM_BUILD_ROOT

%preun
if test $1 = 0
then
  %{__mv} %{_sysconfdir}/cone %{_sysconfdir}/cone.rpmsave
fi

%pre
if test $1 = 1 -a -f %{_sysconfdir}/cone.rpmsave -a ! -f %{_sysconfdir}/cone
then
  %{__mv} %{_sysconfdir}/cone.rpmsave %{_sysconfdir}/cone
fi

%post
%{__perl} %{_datadir}/cone/cone.sysconftool %{_sysconfdir}/cone.dist >/dev/null

%files
%defattr(-,root,root)
%attr(644,root,root) %{_sysconfdir}/cone.dist
%ghost %verify(user group mode) %attr(644,root,root) %{_sysconfdir}/cone
%{_bindir}/*
%{_libexecdir}/cone
%{_datadir}/cone
%{_mandir}/man1/*
%doc ABOUT-NLS ChangeLog README NEWS AUTHORS COPYING COPYING.GPL

%files devel
%defattr(-,root,root)
%{_libdir}/*.a
%{_libdir}/*.la
%{_mandir}/man[35]/*
%{_includedir}/libmail
%doc cone/html

%changelog
* Wed Apr 14 2004 Mr. Sam <sam@email-scan.com>
- Replace BuildPreReq: with BuildRequires, +other fixes.
- Remove duplicate html docs, move them to -devel subpackage.

* Mon Sep  1 2003 Mr. Sam <sam@email-scan.com>
- Fix for Red Hat 9+

* Sat Jul 26 2003 Mr. Sam 0.52
- Use wide-char compatible ncurses in current RH Beta.

* Wed Mar  5 2003 Mr. Sam <mrsam@courier-mta.com>
- Initial build.

