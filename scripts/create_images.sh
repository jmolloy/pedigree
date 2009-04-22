#
# Copyright (c) 2008 James Molloy, James Pritchett, Jšrg PfŠhler, Matthew Iselin
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
HDIMG=hdd_ext2.img
HDOFF=32256
SRCDIR=.
HDFILES=$@

init() {
    mkdir -p $MOUNTPT
    sudo mount -o loop,offset=$OFF $IMG $MOUNTPT
}

fini() {
    sudo umount $MOUNTPT
    rm -rf $MOUNTPT
}


if sudo which losetup >/dev/null 2>&1; then
  MOUNTPT=floppytmp
  cp $SRCDIR/../images/floppy_ext2.img $SRCDIR/floppy.img

  init;

  # Floppy mounted - transfer files.
  sudo cp $SRCDIR/src/system/kernel/kernel $MOUNTPT/kernel
  sudo cp $SRCDIR/initrd.tar $MOUNTPT/initrd.tar

  # Unmount floppy.
  fini;

  # ----------------

  # Mount HDD (ext2)
  tar -xzf ../images/hdd_ext2.tar.gz
  IMG=$HDIMG
  OFF=$HDOFF
  MOUNTPT=hddtmp
  
  init;

  sudo cp -a $SRCDIR/../images/i686-elf/. $MOUNTPT/
#  sudo svn export --force $SRCDIR/../images/i686-elf $MOUNTPT/
#  sudo svn export --force $SRCDIR/../images/ppc-elf $MOUNTPT/

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules
  sudo mkdir -p $MOUNTPT/tmp

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

  # ----------------

  # Mount HDD (fat16)
  tar -xzf ../images/hdd_fat16.tar.gz
  #IMG=hdd_16h_63spt_100c_fat16.img
  IMG=hdd_fat16.img
  OFF=$HDOFF
  MOUNTPT=hddtmp2
  init;

  sudo touch $MOUNTPT/.pedigree-root

  OLD=$PWD
  
  cd $SRCDIR/../images/i686-elf
  tar -chf tmp.tar *
  ARCLOC=$PWD
  cd $OLD/$MOUNTPT
  sudo tar -xof $ARCLOC/tmp.tar
  cd $ARCLOC
  rm tmp.tar
  
  cd $OLD
  
  #tar -cf tmp.tar $SRCDIR/../images/i686-elf/*
  #tar -xf tmp.tar $MOUNTPT
  #sudo cp -r $SRCDIR/../images/i686-elf $MOUNTPT
#  sudo svn export --force $SRCDIR/../images/i686-elf $MOUNTPT/
#  sudo svn export --force $SRCDIR/../images/ppc-elf $MOUNTPT/

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules
  sudo mkdir -p $MOUNTPT/tmp

  # This is a root filesystem.
#   sudo touch $MOUNTPT/.pedigree-root

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

#  if which qemu-img >/dev/null; then
#    sudo qemu-img convert $SRCDIR/hdd_ext2.img -O vmdk $SRCDIR/hdd_ext2.vmdk
#  fi

#  if which VBoxManage >/dev/null; then
#    if [ -f $SRCDIR/hdd_16h_63_spt_100c.vdi ]; then
#      rm $SRCDIR/hdd_16h_63_spt_100c.vdi
#    fi
#    VBoxManage convertdd $SRCDIR/hdd_16h_63spt_100c.img $SRCDIR/hdd_ext.vdi
#  fi

elif which mcopy >/dev/null 2>&1; then

  cp ../images/floppy_fat.img ./floppy.img

  sh ../scripts/mtsetup.sh ./floppy.img > /dev/null 2>&1

  mcopy -Do ./src/system/kernel/kernel A:/
  mcopy -Do ./initrd.tar A:/

  tar -xzf ../images/hdd_fat16.tar.gz

  sh ../scripts/mtsetup.sh ./hdd_fat16.img > /dev/null 2>&1

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

  mkdir -p ./tmp

  # This was cp -a, but OS X doesn't have -a. On linux -a is equivalent to:
  # -dpR. OS X doesn't have -d, but -d is the same as -P --preserve=links.
  # OS X doesn't have --preserve either, but it is how -P is implemented, so
  # -a is best replaced with -pPR.
  cp -p $SRCDIR/../images/i686-elf/.bashrc ./tmp
  cp -pPR $SRCDIR/../images/i686-elf/* ./tmp

  mcopy -Do -s ./tmp/.bashrc C:/
  mcopy -Do -s ./tmp/* C:/

  rm -rf ./tmp

  echo Only creating FAT disk image as \`losetup\' was not found.
else
  echo Not creating disk images because neither \`losetup\' nor \`mtools\' could not be found.
fi
