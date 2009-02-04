#!/usr/bin/perl

use strict;
use warnings;

my $width = 8;
my $height = 16;
my $max_encoding = 128;

my $infile = $ARGV[0];
my $outfile = $ARGV[1];
my %chars;

open(INFILE, "<", $infile) or die("Cannot open input file.");
open(OUTFILE, ">", $outfile) or die("Cannot open output file.");

sub start_char {
  my ($name) = @_;
  my $encoding;
  my $str = '';

  while (<INFILE>) {

    if ($_ =~ m/ENCODING (.*)/) {
      $encoding = $1;
      if ($encoding > $max_encoding || $encoding == -1) {
        return;
      }
    }
    elsif ($_ =~ m/BITMAP/) {
      last;
    }
  }

  $str = "  /* $name ($encoding) */\n";

  my $num = 0;
  my $str2;
  while (<INFILE>) {
    if ($_ =~ m/ENDCHAR/) {
      last;
    }
    else {
      $_ =~ m/^(..)/;
      $str2 .= "  0x$1,\n";
      $num++;
    }
  }

  while ($num != $height) {
    $str .= "  0x00,\n";
    $num++;
  }
  $str .= $str2;

  $str .= "\n";

  $chars{$encoding} = $str;
}

while (<INFILE>) {
  if ($_ =~ m/FACE_NAME "(.*)"/) {
    print OUTFILE "/* Font name: $1 */\n";
    print OUTFILE "unsigned char ppc_font[] = {\n";
  }
  elsif ($_ =~ m/STARTCHAR (.*)/) {
    start_char($1);
  }
}

my $str;
my $this = 0;
for my $key (sort {$a <=> $b } keys %chars) {
  while ($this != $key) {
    $str .= $chars{32};
    $this ++;
  }
  $str .= $chars{$key};
  $this++;
}

# Remove ',\n'.
chop $str;
chop $str;
chop $str;

$str .= "\n};\n";
print OUTFILE $str;
