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

use strict;
use warnings;

my $libc = shift @ARGV;
my $libm = shift @ARGV;
my $glue = shift @ARGV;
my $include = shift @ARGV;
my $ld = shift @ARGV;
my $binary_dir = shift @ARGV;
my $libgcc = shift @ARGV;

`mkdir -p /tmp/pedigree-tmp`;
`cp $libc /tmp/pedigree-tmp/libc.a`;
`cd /tmp/pedigree-tmp; ar x /tmp/pedigree-tmp/libc.a`;
`rm /tmp/pedigree-tmp/libc.a`;
`cp $glue /tmp/pedigree-tmp`;
`cd /tmp/pedigree-tmp; ar cru libc.a *.o`;
`cp /tmp/pedigree-tmp/libc.a ./libc.a`;
`rm /tmp/pedigree-tmp/lib_a-init.o`; # We don't want this in the .so - it contains shite that references hidden symbols that is never used.
`rm /tmp/pedigree-tmp/lib_a-popen.o /tmp/pedigree-tmp/lib_a-getpass.o`; # TMP - implement the functions this requires!
`$ld -nostdlib -Wl,-Bsymbolic -Wl,-shared -Wl,-soname,libc.so -L$libgcc -o libc.so /tmp/pedigree-tmp/*.o -lgcc`;
`rm -rf /tmp/pedigree-tmp/*`;
`cp $libm /tmp/pedigree-tmp/libm.a`;
`cd /tmp/pedigree-tmp; ar x /tmp/pedigree-tmp/libm.a`;
`rm /tmp/pedigree-tmp/libm.a`;
`$ld -nostdlib -Wl,-Bsymbolic -Wl,-shared -Wl,-soname,libm.so -L$libgcc -o libm.so /tmp/pedigree-tmp/*.o -lgcc`;
`cp $libm ./libm.a`;
`rm -rf /tmp/pedigree-tmp`;
`rm -rf ./include-posix`;
`cp -r $include ./include-posix`;
