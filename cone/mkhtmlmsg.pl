#!/usr/bin/perl

my $pid=open(PIPE, "-|");

die "fork: $!\n" unless defined $pid;

unless ($pid)
{
    my $l;

    while (defined($l=<STDIN>))
    {
	if ($l =~ /><DIV/)
	{
	    chomp $l;
	    $l .= " ";
	}

	if ($l =~ /<TITLE/)
	{
	    until ($l =~ /\/TITLE/)
	    {
		my $ll=<STDIN>;

		last if ! defined $ll;

		chomp $l;
		$l .= " $ll";
	    }
	}
	print $l;
    }
    exit 0;
}

my $html;
my $title="";

my $l;

while (defined($l=<PIPE>))
{
    $title=$1 if $l =~ /<TITLE\s*>(.*)<\/TITLE/ && $1 !~ /^\s*$/;
    last if $l =~ /<DIV CLASS=\"(CHAPTER|PART)\"/;
}
$l =~ s/^>//;
$html .= $l;

while (defined($l=<PIPE>))
{
  if ($l =~ />(.*)<\/H[123456789]/ && $1 !~ /^\s*$/ && !$title)
    {
      $title=$1;
    }

  if ($l =~ /<DIV CLASS=\"TOC\"/)
    {
      do
	{
	  $l=<PIPE>;
	} while (defined $l && $l !~ /\/DIV/);
    }

  last if $l =~ /<DIV CLASS=\"(NAVFOOTER)\"/;
  $html .= $l;
}
$html .= ">\n";

$title =~ s/\s+/ /g;

print "From nobody\@localhost " . (localtime) . "\n";
print "From: Online Help <nobody\@localhost>\n";
print "Subject: $title\n";
print "Mime-Version: 1.0\n";
print "Content-Type: text/html; charset=iso-8859-1\n";
print "\n";
print "<body><html>$html</body></html>\n\n\n";
