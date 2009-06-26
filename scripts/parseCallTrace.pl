#!/usr/bin/perl

use strict;
use warnings;

my $min = 0; #1000;

my $filename = shift @ARGV;

open my $fd, "<", $filename or die ("Unable to open file $filename.");

my %funcalls;

my $line = 0;
my $duff_lines  = 0;
while (<$fd>) {
    print "Processing entry $line...\n" if ($line % 10000 == 0);

    # The output routines use a highly simplified version of hex -
    # '0' + number. That means digits a-f are mapped incorrectly in ascii.
    s/:/a/g;
    s/;/b/g;
    s/\</c/g;
    s/=/d/g;
    s/\>/e/g;
    s/\?/f/g;

    my ($function, $called_from) = m/E(.*) (.*)/;
    if (defined $function && defined $called_from) {
	my $cf = $funcalls{$function};
	if (defined $cf) {
	    $cf->{$called_from} ++;
	} else {
	    $funcalls{$function} = {$called_from => 1};
	}
    } else {
	$duff_lines ++;
    }
    $line ++;
}

#foreach my $func (keys %funcalls) {
#    my $hash_ref = $funcalls{$func};
#    my %hash = %$hash_ref;
#    foreach my $called_by (keys %hash) {
#	my $val = $hash{$called_by};
#	print "$called_by -> $func ($val)\n";
#    }
#}

# Perform symbol lookup.
my %lookups;
print "Performing lookups...\n";

sub do_lookup {
    my $addr = shift;

    if (defined $lookups{$addr}) {
	return $lookups{$addr};
    }

    my $output = `addr2line -Cf -e src/system/kernel/kernel $addr`;
    my ($func) = $output =~ m/^(.*)$/m;
    $func = substr($func, 0, 30);
    $lookups{$addr} = $func;

    return $func;
}

my %funcalls_funcs;

foreach my $func (keys %funcalls) {
    my $hash_ref = $funcalls{$func};
    my %hash = %$hash_ref;

    # Attempt to prune minimal nodes.
    my $total = 0;
    foreach my $called_by (keys %hash) {
	my $val = $hash{$called_by};
	$total += $val;
    }
    if ($total > $min) {
	# We can now amalgamate called_by - multiple addresses in the same function
	# could call this function, so lookup and add to a new hash, hashed on name.
	my %called_by_funcs;

	foreach my $called_by (keys %hash) {
	    my $val = $hash{$called_by};
	    $called_by_funcs{do_lookup($called_by)} += $val;
	}

	$funcalls_funcs{do_lookup($func)} = \%called_by_funcs;
    }
}

# Generate .dot file.
print "Generating DOT file...\n";

open my $fd2, ">", "pedigree.dot";

print $fd2 "digraph G {\n";
foreach my $func (keys %funcalls_funcs) {
    #print $fd2 "\"$func\"\n";
}

print $fd2 "\n";

foreach my $func (keys %funcalls_funcs) {
    my $hash_ref = $funcalls_funcs{$func};
    my %hash = %$hash_ref;
    foreach my $called_by (keys %hash) {
	my $val = $hash{$called_by};
	if ($val > $min) {
	    print $fd2 "\"$called_by\" -> \"$func\" [label=\"$val\"]\n";
	}
    }
}

print $fd2 "\n}\n";
