#!/usr/bin/perl

open my $r, "<", "buildFlags";
open my $w, ">", "buildFlags.mk";

while (<$r>) {
  next if $_ =~ m/^#/;
  print $w "export $_\n";
  print $w "CMD_FLAGS += -D$_\n";
}
