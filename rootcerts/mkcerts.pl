#!/usr/bin/perl
#
# $Id: mkcerts.pl,v 1.1 2001/04/07 21:30:26 mrsam Exp $
#
# Copyright 2001 Double Precision, Inc.  See COPYING for
# distribution information.
#
# Create certs.sh by swiping root CA certs from Mozilla CVS:
#
# CVSROOT:  :pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot
#
# File:     mozilla/security/nss/lib/ckfw/builtins/certdata.txt
#
# Assumes that builtins is the subdirectory
#

# Create pem scratch directory

if (-d "pem")
{
    opendir(D, "pem") || die "$!\n";
    while (defined ($f=readdir(D)))
    {
	next if substr($f, 0, 1) eq ".";
	unlink "pem/$f";
    }
    closedir(D);
    rmdir("pem");
}

mkdir("pem");

# Swipe DER data from certdata.txt, and manually convert it from octal

$filenum=0;
open (F, "builtins/certdata.txt") || die "$!\n";

while (<F>)
{
    next unless /^CKA_VALUE MULTILINE_OCTAL/;

    $filename="pem/pem$filenum.pem";
    ++$filenum;

    open (D, "| openssl x509 -inform der -text -outform pem >$filename")
	|| die "$!\n";

    while (<F>)
    {
	last if /^END/;
	chomp;

	$line="";

	grep {
	    my $x=$_;
	    if (length($x) == 3)
	    {
		eval ('$byte' . "=0$x;");
		$line .= chr($byte);
	    }
	} split (/\\/, $_);
	print D $line;
    }
    close(D);

    # Ok, now standardize the filename by grabbing the subject, and going
    # from there.

    open (D, "openssl x509 -subject <$filename |") || die "$!\n";

    $subject="";

    while (<D>)
    {
	next unless /^subject=(.*)/;
	$subject=$1;
    }
    close(D);

    # Decompose subject

    my %var;

    $subject =~ s/\/([a-zA-Z]*=)/\n$1/g;

    grep { $var{$1}=$2 if /^([A-Z]*)=(.*)/; } split (/\n/, $subject);

    # Grab CN.  If missing, grab O and OU

    my $n=$var{'CN'};

    $n="$var{'O'} $var{'OU'}" unless $n;

    # Put everything to lower case.  Replace non-alnum with dashes.  Collapse
    # multiple dashes.

    $n =~ tr/[A-Z]/[a-z]/;
    $n =~ s/[^0-9a-z]/-/g;
    $n =~ s/--*/-/g;
    $n =~ s/^-*//;
    $n =~ s/-*$//;

    # Truncate some long subjects down

    $subject="";

    grep {
	my $s=$_;

	if (length($subject) < 30)
	{
	    $subject .= "-" unless $subject eq "";
	    $subject .= $s;
	}
    } split (/\-/, $n);

    # Use what we have for a filename.  Several certs have similar subjects,
    # so take care not to overwrite.

    $new_filename="pem/$subject-00.pem";
    $c=0;

    while ( -f $new_filename )
    {
	$new_filename = sprintf("pem/$subject-%02d.pem", ++$c);
    }

    rename($filename, $new_filename);
}

#
# Ok, now take certs with unique subjects, and just drop the -00
#

opendir(D, "pem") || die "$!\n";
while (defined ($f=readdir(D)))
{
    next unless $f =~ /(.*)-00\.pem$/;

    my $base=$1;

    next if -f "pem/$base-01.pem";

    rename ("pem/$base-00.pem", "pem/$base.pem");
}
closedir(D);

# Now, read what we have, and dump it to stdout

opendir(D, "pem") || die "$!\n";

grep {
    my $f=$_;

    print "cat >certs/$f <<...EOF...\n";
    open (F, "pem/$f") || die "$!\n";
    while (<F>)
    {
	print $_;
    }
    close (F);
    print "...EOF...\n";
}
grep(/^[^\.]/, sort readdir(D));
closedir(D);
