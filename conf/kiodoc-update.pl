#! /usr/bin/perl -w
#
# $Id$

chomp(my $htmldir = `kde-config --expandvars --install html`);

sub from($) {
	open(F, "<$_[0]") || die "Can't open $_[0] for read: $!\n";
	chomp(my @lines = <F>);
	close(F);
	return @lines;
}

sub collect_specified($) {
	my $cmd = "find $htmldir/*/kioslave -name $_[0].docbook";
	open(F, "$cmd |") || die "Can't run $cmd: $!\n";
	chomp(my @lines = <F>);
	close F;
	return grep(!/HTML\/default/, @lines);
}

sub get_id($) {
	my @lines = from($_[0]);
	foreach (@lines) {
		if (/\<article\s+.*id=\"(.+)\".*\>/i) {
			return "$1";
		}
		last if /\<title\>/i;
	}
	return "";
}

sub usage {
	die "Usage: kiodoc-update -a|-r kioslaveName";
}

sub add_doc($) {
	my @files = collect_specified($_[0]);
	return if ($#files lt 0);
	foreach $idx (@files) {
		my $id = get_id($idx);
		die "Can't read ID attribute in $idx\n" if ($id eq "");
		my $ed = '<!ENTITY kio-' . $id . ' SYSTEM "' .
			"$_[0].docbook" . '">';
		my $er = '&kio-' . $id . ';';
		$idx =~ s/$_[0].docbook/index.docbook/;
		my @lines = from("$idx");
		my $state = 0;
		my @out = ();
		foreach (@lines) {
			$state = 3 if (($state == 2) && (/\&kio-/));
			$state = 1 if (($state == 0) && (/\<\!ENTITY kio-/));
			if ($state == 1) {
				if (/% addindex/) {
					push @out, "$ed\n";
					$state = 2;
				}
				if ($_ gt $ed) {
					push @out, "$ed\n";
					$state = 2;
				}
			}
			elsif ($state == 3) {
				if ($_ gt $er) {
					push @out, "$er\n";
					$state = 4;
				}
				if (/<\/part\>/i) {
					push @out, "$er\n";
					$state = 4;
				}
			}
			next if (/^$er$/);
			next if (/^$ed$/);
			push @out, "$_\n";
		}
		open(F, ">$idx") || die "Can't open $idx for write: $!\n";
		print F @out;
		close(F);
	}
}

sub remove_doc($) {
	my @files = collect_specified($_[0]);
	return if ($#files lt 0);
	my $re = "kio-" . get_id($files[0]) . '[\s;]';
	foreach $idx (@files) {
		$idx =~ s/$_[0].docbook/index.docbook/;
		my @lines = from($idx);
		@lines = grep(!/$re/, @lines);
		open(F, ">$idx") || die "Can't open $idx for write: $!\n";
		print F join("\n", @lines) . "\n";
		close(F);
	}
}

my $worker = \&usage;

while (defined ($ARGV[0])) {
	$_ = shift;
	if (/^-a$/) {
		$worker = \&add_doc;
	}
	elsif (/^-r$/) {
		$worker = \&remove_doc;
	}
	else {
		&$worker($_);
	}
}
