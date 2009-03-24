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

IMG=floppy.img
OFF=0
HDIMG=hdd_16h_63spt_100c.img
HDOFF=32256
SRCDIR=.
MOUNTPT="/tmp/$$"
HDFILES=$@

init() {
    DEV=`sudo losetup -j $IMG | cut -f1 -d:`
    if test "x$DEV" = x ; then
        DEV=`sudo losetup --verbose --find -o $OFF $IMG | \
             awk '/\/dev\/loop/{print $4}'`
    fi
    mount | grep $DEV > /dev/null || {
        sudo mount $DEV $MOUNTPT
    }
}

fini() {
  if which losetup >/dev/null 2>&1; then
    DEV=`sudo losetup -j $IMG | cut -f1 -d:`
    if test "x$DEV" = x ; then
        return 0
    fi
    mount | grep $DEV > /dev/null && {
        sudo umount $DEV
    }
    sudo losetup -d $DEV
  fi
}

trap fini EXIT

if which losetup >/dev/null 2>&1; then
  mkdir -p $MOUNTPT

  cp $SRCDIR/../images/floppy_ext2.img $SRCDIR/floppy.img

  init

  # Floppy mounted - transfer files.
  sudo cp $SRCDIR/src/system/kernel/kernel $MOUNTPT/kernel
  sudo cp $SRCDIR/initrd.tar $MOUNTPT/initrd.tar

  # Unmount floppy.
  fini

  # Mount HDD (ext2)
  cp $SRCDIR/../images/hdd_16h_63spt_100c.img $SRCDIR/hdd_16h_63spt_100c.img
  IMG=$HDIMG
  OFF=$HDOFF
  init

  sudo svn export --force $SRCDIR/../images/i686-elf $MOUNTPT/

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules

  # This is a root filesystem.
  sudo touch $MOUNTPT/.pedigree-root

  # Transfer files.
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/src/user/$f/$BINARY ]; then
      sudo cp $SRCDIR/src/user/$f/$BINARY $MOUNTPT/$f
    fi
    if [ -f $SRCDIR/src/user/$f/lib$BINARY.so ]; then
      sudo cp $SRCDIR/src/user/$f/lib$BINARY.so $MOUNTPT/libraries/lib$BINARY.so
    fi
  done
  sudo cp $SRCDIR/libc.so $MOUNTPT/libraries
  sudo cp $SRCDIR/libm.so $MOUNTPT/libraries

  sudo mkdir -p $MOUNTPT/etc/terminfo/v
#  sudo cp $SRCDIR/../scripts/termcap $MOUNTPT/etc
  sudo cp $SRCDIR/../scripts/vt100 $MOUNTPT/etc/terminfo/v/

  fini;

  # Mount HDD (fat16)
  cp $SRCDIR/../images/hdd_16h_63spt_100c_fat16.img $SRCDIR/hdd_16h_63spt_100c_fat16.img
  IMG=hdd_16h_63spt_100c_fat16.img
  OFF=$HDOFF
  init

  sudo touch $MOUNTPT/.pedigree-root

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules

  # Transfer files.
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/src/user/$f/$BINARY ]; then
      sudo cp $SRCDIR/src/user/$f/$BINARY $MOUNTPT/$f
    fi
    if [ -f $SRCDIR/src/user/$f/lib$BINARY.so ]; then
      sudo cp $SRCDIR/src/user/$f/lib$BINARY.so $MOUNTPT/libraries/lib$BINARY.so
    fi
  done
  sudo cp $SRCDIR/libc.so $MOUNTPT/libraries
  sudo cp $SRCDIR/libm.so $MOUNTPT/libraries

  fini;

  if which qemu-img >/dev/null; then
    sudo qemu-img convert $SRCDIR/hdd_16h_63spt_100c.img -O vmdk $SRCDIR/hdd_16h_63spt_100c.vmdk
  fi

  if which VBoxManage >/dev/null; then
    if [ -f $SRCDIR/hdd_16h_63_spt_100c.vdi ]; then
      rm $SRCDIR/hdd_16h_63_spt_100c.vdi
    fi
    VBoxManage convertdd $SRCDIR/hdd_16h_63spt_100c.img $SRCDIR/hdd_16h_63_spt_100c.vdi
  fi

elif which mcopy >/dev/null 2>&1; then

  cp ../images/floppy_fat.img ./floppy.img
  
  ../scripts/mtsetup.sh ./floppy.img > /dev/null 2>&1
  
  mcopy -Do ./src/system/kernel/kernel A:/
  mcopy -Do ./initrd.tar A:/

  # cp ../images/hdd_16h_63spt_100c.img .
  cp ../images/hdd_16h_63spt_100c_fat16.img .
  
  ../scripts/mtsetup.sh ./hdd_16h_63spt_100c_fat16.img > /dev/null 2>&1

  touch ./.pedigree-root

  mmd -Do applications libraries modules etc usr tmp

  # make this the root disk
  mcopy -Do ./.pedigree-root C:/

  rm ./.pedigree-root

  mcopy -Do $SRCDIR/../scripts/termcap C:/etc
  
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/src/user/$f/$BINARY ]; then
      mcopy -Do $SRCDIR/src/user/$f/$BINARY C:/$f
    fi
    if [ -f $SRCDIR/src/user/$f/lib$BINARY.so ]; then
      mcopy -Do $SRCDIR/src/user/$f/lib$BINARY.so C:/libraries
    fi
  done

  mcopy -Do $SRCDIR/libc.so C:/libraries
  mcopy -Do $SRCDIR/libm.so C:/libraries

  #mmd -Do C:/etc/terminfo
  #mmd -Do C:/etc/terminfo/v
  #mcopy -Do $SRCDIR/../scripts/vt100 C:/etc/terminfo/v
  
  svn export --force $SRCDIR/../images/i686-elf ./tmp
  
  mcopy -Do -s ./tmp/.bashrc C:/
  mcopy -Do -s ./tmp/* C:/
  
  rm -rf ./tmp
  
  #mcopy -Do -s $SRCDIR/../images/i686-elf/.bashrc C:/
  #mcopy -Do -s $SRCDIR/../images/i686-elf/* C:/

  # mcopy -Do -s $SRCDIR/../images/i686-elf/* C:/

  echo Only creating FAT disk image as \`losetup\' was not found.
else
  echo Not creating disk images because neither \`losetup\' nor \`mtools\' could not be found.
fi
