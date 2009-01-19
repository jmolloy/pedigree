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
    DEV=`sudo losetup -a | grep $IMG | cut -f1 -d:`
    if test "x$DEV" = x ; then
        DEV=`sudo losetup --verbose --find -o $OFF $IMG | \
             awk '/\/dev\/loop/{print $4}'`
    fi
    mount | grep $DEV > /dev/null || {
        sudo mount $DEV $MOUNTPT
    }
}

fini() {
    DEV=`sudo losetup -a | grep $IMG | cut -f1 -d:`
    if test "x$DEV" = x ; then
        return 0
    fi
    mount | grep $DEV > /dev/null && {
        sudo umount $DEV
    }
    sudo losetup -d $DEV
}
trap fini EXIT

if which losetup >/dev/null; then
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

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules

  # This is a root filesystem.
  sudo touch $MOUNTPT/.pedigree-root

  # Transfer files.
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    sudo cp $SRCDIR/src/user/$f/$BINARY $MOUNTPT/$f
  done
  sudo cp $SRCDIR/libc.so $MOUNTPT/libraries
  sudo cp $SRCDIR/libm.so $MOUNTPT/libraries

  fini;

  # Mount HDD (fat16)
  cp $SRCDIR/../images/hdd_16h_63spt_100c_fat16.img $SRCDIR/hdd_16h_63spt_100c_fat16.img
  IMG=hdd_16h_63spt_100c_fat16.img
  OFF=$HDOFF
  init

  # Create required directories.
  sudo mkdir -p $MOUNTPT/applications
  sudo mkdir -p $MOUNTPT/libraries
  sudo mkdir -p $MOUNTPT/modules

  # Transfer files.
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    sudo cp $SRCDIR/src/user/$f/$BINARY $MOUNTPT/$f
  done
  sudo cp $SRCDIR/libc.so $MOUNTPT/libraries
  sudo cp $SRCDIR/libm.so $MOUNTPT/libraries

  fini;

  if which qemu-img >/dev/null; then
    sudo qemu-img convert $SRCDIR/hdd_16h_63spt_100c.img -O vmdk $SRCDIRhdd_16h_63spt_100c.vmdk
  fi

  if which VBoxManage >/dev/null; then
    if [ -f $SRCDIR/hdd_16h_63_spt_100c.vdi ]; then
      rm $SRCDIR/hdd_16h_63_spt_100c.vdi
    fi
    VBoxManage convertdd $SRCDIR/hdd_16h_63spt_100c.img $SRCDIR/hdd_16h_63_spt_100c.vdi
  fi

else
  echo Not creating disk images because \`losetup\' could not be found.
fi
