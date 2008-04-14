#!/usr/bin/perl

#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Parses a "buildFlags" file.
# Format is:
#   Lines beginning with a # are comments.
#   All other lines are build rules - "LEFT: RIGHT1 RIGHT2 ..." - if left is defined, then define right1 and right2...
#   If a build rule is encapsulated in [], e.g. "[LEFT]:", then this variable is internal and will not be
#   displayed in the GUI.
#   If a build rule is encapsulated in {}, e.g. "{LEFT}:", then this variable will be marked as 'advanced'.

use strict;
use warnings;

open my $r, "<", "buildFlags" or die("Couldn't open file 'buildFlags' for reading.");
open my $w, ">", "buildFlags.cmake" or die("Couldn't open file 'buildFlags.cmake' for writing.");

my %rule_rhs;
my %rule_isadvanced;
my %rule_isinternal;

while (<$r>) {
  next if $_ =~ m/^#/; # Detect comments.
  next if $_ =~ m/^[ \t]*$/; # Detect empty lines.
  
  my $line = $_;
  my $right = '';
  my $left = '';
  my $internal = 0;
  my $advanced = 0;
  if ($line =~ m/^(\w+): ?(.*)$/) {
    $left = $1;
    $right = $2;
  }
  elsif ($line =~ m/^\[(\w+)\]: ?(.*)$/) {
    $internal = 1;
    $left = $1;
    $right = $2;
  }
  elsif ($line =~ m/^\{(\w+)\}: ?(.*)$/) {
    $advanced = 1;
    $left = $1;
    $right = $2;
  }
  
  $rule_rhs{$left} = $right;
  $rule_isadvanced{$left} = $isadvanced;
  $rule_isinternal{$left} = $isinternal;
}

# Start of output.

print $w "# Loop five times - hopefully then no more variables will be being changed.\n";
print $w "foreach(CHANGED RANGE 5)\n";
print $w "  set(CHANGED NO)\n";
print $w "\n";

foreach $rule (keys %rule_rhs) {
print $w "  # Rule: $rule.\n";
print $w "  if($rule)\n";
print $w "    set($rule-PREV ON)\n"
print $w "    # Set flags $rule_rhs{$rule}.\n";
foreach (split (/ +/, $rules_rhs{$rule})) {
print $w "    set($_ ON CACHE BOOL \"$_\" FORCE)\n";
print $w "    set(DEFINES \${DEFINES} $_)\n";
}
print $w "  else($rule)\n";
print $w "    if($rule-PREV STREQUAL ON)\n";
print $w "      set($rule-PREV OFF)\n";
foreach (split (/ +/, $rules_rhs{$rule})) {
print $w "      set($_ OFF CACHE BOOL  \"$_\" FORCE)\n";
print $w "      list(REMOVE_ITEM DEFINES $_)\n";
}
print $w "    endif($rule-PREV OFF)\n";
print $w "  endif($rule)\n";
}

print $w "\n";
print $w "endforeach(CHANGED RANGE 5)\n";

close $r;
close $w;
