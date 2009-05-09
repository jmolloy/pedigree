#
# Copyright (c) 2008 James Molloy, James Pritchett, Jrg Pfhler, Matthew Iselin
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

# Run this in the images/install folder

SRCDIR=../../

echo "Building installer disk images..."

if which mcopy >/dev/null 2>&1; then

  if [ ! -e "./hdd_fat16.img" ]; then
        tar -xzf ../hdd_fat16.tar.gz
  fi

  sh $SRCDIR/scripts/mtsetup.sh ./hdd_fat16.img > /dev/null 2>&1

  touch ./.pedigree-root

  mmd -Do applications libraries etc

  # make this the root disk
  mcopy -Do ./.pedigree-root C:/

  rm ./.pedigree-root

  mcopy -Do $SRCDIR/build/libc.so C:/libraries
  mcopy -Do $SRCDIR/build/libm.so C:/libraries

  mcopy -Do -s ./ramdisk/* C:/

  tar -czf ramdisk.tgz ./hdd_fat16.img
  rm ./hdd_fat16.img

  cp ramdisk.tgz ./disk/
  rm ramdisk.tgz
else
  echo Not creating ramdisk image because \`mtools\' could not be found.
fi

if which mkisofs > /dev/null 2>&1; then
    cp $SRCDIR/build/src/system/kernel/kernel ./disk/boot
    cp $SRCDIR/build/initrd.tar ./disk/boot
    rm -f pedigree-i386.iso
    mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o pedigree-i386.iso disk
else
    echo Not creating bootable ISO becuase \`mkisofs\' could not be found.
fi

