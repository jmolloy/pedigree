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

my $compiler = shift @ARGV;
my @hdd_files = @ARGV;

# Are we making an X86 target?
if ($compiler =~ m/(i686|amd64)-elf/) {
  # Are we on cygwin?
  `cp src/system/kernel/kernel kernel`;
  if ($ENV{TERM} =~ m/cygwin/i) {
    `cp ../images/floppy_fat.img ./floppy.img`;
    `mcopy -Do -i  ./floppy.img src/system/kernel/kernel ::/`;
  } else {
    `mkdir -p /tmp/pedigree-image`;
    `cp ../images/floppy_ext2.img ./floppy.img`;
    `sudo /sbin/losetup /dev/loop0 ./floppy.img`;
    `sudo mount -t ext2 /dev/loop0 /tmp/pedigree-image`;
    `sudo cp src/system/kernel/kernel /tmp/pedigree-image/kernel`;
    `sudo cp initrd.tar /tmp/pedigree-image/initrd.tar`;
    `sudo umount /dev/loop0`;
    `sudo /sbin/losetup -d /dev/loop0`;
    `sudo cp ../images/hdd_16h_63spt_100c.img ./`;
    `sudo /sbin/losetup -o 32256 /dev/loop0 ./hdd_16h_63spt_100c.img`;
    `sudo mount -t ext2 /dev/loop0 /tmp/pedigree-image`;
    `sudo mkdir -p /tmp/pedigree-image/applications`;
    `sudo mkdir -p /tmp/pedigree-image/libraries`;
    foreach my $file (@hdd_files) {
	my ($binary) = ($file =~ m/([^\/]+)$/);
	`sudo cp src/user/$file/$binary /tmp/pedigree-image/$file`;
    }
    `sudo cp libc.so /tmp/pedigree-image/libraries/libc.so`;
    `sudo cp libm.so /tmp/pedigree-image/libraries/libm.so`;
    `sudo umount /dev/loop0`;
    `sudo /sbin/losetup -d /dev/loop0`;
    `which qemu-img`;
    if ($? == 0) {
	`sudo qemu-img convert hdd_16h_63spt_100c.img -O vmdk hdd_16h_63spt_100c.vmdk`;
    }
    `which VBoxManage`;
    if ($? == 0) {
	`VBoxManage convertdd hdd_16h_63spt_100c.img hdd_16h_63_spt_100c.vdi`;
    }
  }
}

# How about a mips target?
if ($compiler =~ m/mips64el-elf/) {
  `../compilers/dir/mips64el-elf/bin/objcopy src/system/boot/mips/bootloader -O srec ./bootloader.srec`;
  `cp src/system/boot/mips/bootloader ./bootloader`;
}

# How about an arm target?
if ($compiler =~ m/arm-elf/) {
  `cp src/system/boot/arm/bootloader ./bootloader`;
}

# PPC?
if ($compiler =~ m/ppc/) {
  `cp src/system/boot/ppc/bootloader ./bootloader`;
  # HFS image.
  unless (-f "hdd_ppc_16h_63spt_100c.img") {
    `cp ../images/hdd_ppc_16h_63spt_100c.img .`;
  }
  `hmount hdd_ppc_16h_63spt_100c.img`;
  `hcopy ./bootloader :pedigree`;
  `humount`;

  # EXT2 image.
#  unless (-f "hdd_16h_63spt_100c.img") {
    `cp ../images/hdd_16h_63spt_100c.img .`;
#  }
  `mkdir -p /tmp/pedigree-image`;
  `sudo /sbin/losetup -o 32256 /dev/loop0 ./hdd_16h_63spt_100c.img`;
  `sudo mount -t ext2 /dev/loop0 /tmp/pedigree-image`;
  `sudo mkdir -p /tmp/pedigree-image/applications`;
  `sudo mkdir -p /tmp/pedigree-image/libraries`;
  foreach my $file (@hdd_files) {
      my ($binary) = ($file =~ m/([^\/]+)$/);
      `sudo cp src/user/$file/$binary /tmp/pedigree-image/$file`;
  }
  `sudo cp libc.so /tmp/pedigree-image/libraries/libc.so`;
  `sudo cp libm.so /tmp/pedigree-image/libraries/libm.so`;
  `sudo cp /home/james/bash-3.2/build-ppc/bash /tmp/pedigree-image/bash`;
  `sudo umount /dev/loop0`;
  `sudo /sbin/losetup -d /dev/loop0`;
}

exit 0;
