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
HDIMG=hdd_ext2.img
HDOFF=32256
SRCDIR=$1
HDFILES="apptest login syscall-test net-test netconfig TUI"

init() {
    mkdir -p $MOUNTPT
    sudo mount -o loop,offset=$OFF $IMG $MOUNTPT
}

fini() {
    sudo umount $MOUNTPT
    rm -rf $MOUNTPT
}

export PATH=$PATH:/sbin
if which losetup >/dev/null 2>&1; then
  MOUNTPT=floppytmp
  cp $SRCDIR/../images/floppy_ext2.img $SRCDIR/floppy.img

  init;

  # Floppy mounted - transfer files.
  sudo cp $SRCDIR/kernel/kernel $MOUNTPT/kernel
  sudo cp $SRCDIR/initrd.tar $MOUNTPT/initrd.tar
  sudo cp $SRCDIR/config.db $MOUNTPT/config.db

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
  sudo cp $SRCDIR/config.db $MOUNTPT/.pedigree-root

  # Transfer files.
  sudo cp $SRCDIR/apps/* $MOUNTPT/applications
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

#  OLD=$PWD

#  sudo cp -a $SRCDIR/../images/i686-elf/. $MOUNTPT/
  
#  cd $SRCDIR/../images/i686-elf
#  tar -chf tmp.tar *
#  ARCLOC=$PWD
#  cd $OLD/$MOUNTPT
#  sudo tar -xof $ARCLOC/tmp.tar
#  cd $ARCLOC
#  rm tmp.tar
  
#  cd $OLD
  
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

  # Transfer files.
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/apps/$BINARY ]; then
      sudo cp $SRCDIR/apps/$BINARY $MOUNTPT/applications/
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

elif which fartsandlove >/dev/null 2>&1; then # OS X
    ## Floppy
    cp ../images/floppy_fat.img ./floppy.img
    mkdir -p floppytmp
    export devid=`hdid -nomount ./floppy.img`
    mount -t msdos $devid floppytmp
    cp $SRCDIR/src/system/kernel/kernel floppytmp/kernel
    cp $SRCDIR/initrd.tar floppytmp/initrd.tar
    umount $devid
    rm -r floppytmp
    ## End Floppy
    
    ## HDD (fat16)
    tar -xzf ../images/hdd_fat16.tar.gz
    mkdir -p hddtmp2
    export devid=`hdid -nomount ./hdd_fat16.img | head -n 2 | tail -n 1 | awk '{split($0, s); print s[1] }'`
    mount -t msdos $devid hddtmp2
    touch hddtmp2/.pedigree-root
    # ^-- Required, or pedigree won't be able to find the image
    
    OLD=$PWD
  
    cd $SRCDIR/../images/i686-elf
    tar -chf tmp.tar *
    ARCLOC=$PWD
    cd $OLD/hddtmp2
    sudo tar -xof $ARCLOC/tmp.tar
    cd $ARCLOC
    rm tmp.tar

    cd $OLD
    
    sudo mkdir -p hddtmp2/applications
    sudo mkdir -p hddtmp2/libraries
    sudo mkdir -p hddtmp2/modules
    sudo mkdir -p hddtmp2/tmp
    
    for f in $@; do
        BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
        if [ -f $SRCDIR/apps/$BINARY ]; then
            sudo cp $SRCDIR/apps/$BINARY hddtmp2/applications/
        fi
    done
    
    sudo cp $SRCDIR/libc.so hddtmp2/libraries
    sudo cp $SRCDIR/libm.so hddtmp2/libraries
    
    sudo mkdir -p hddtmp2/etc/terminfo/v
    sudo cp $SRCDIR/../scripts/vt100 hddtmp2/etc/terminfo/v/
    
    umount $devid
    rm -r hddtmp2
    ## End HDD (fat16)
    
    echo Created floppy and FAT16 HD image using hdiutils
elif which mcopy >/dev/null 2>&1; then

  #if [ ! -e "./floppy.img" ]
  #  then
        cp $SRCDIR/../images/floppy_fat.img $SRCDIR/floppy.img
  # fi

  sh $SRCDIR/../scripts/mtsetup.sh $SRCDIR/floppy.img > /dev/null 2>&1

  mcopy -Do $SRCDIR/kernel/kernel A:/
  mcopy -Do $SRCDIR/initrd.tar A:/

  #if [ ! -e "./hdd_fat16.img" ]
  #  then
        tar -xzf ../images/hdd_fat16.tar.gz
  #fi

  sh $SRCDIR/../scripts/mtsetup.sh ./hdd_fat16.img > /dev/null 2>&1
  
  mmd C:/tmp C:/etc

  # Make this the root disk
  touch ./.pedigree-root
  mcopy -Do ./.pedigree-root C:/

  rm ./.pedigree-root

  mcopy -Do -s $SRCDIR/../images/i686-elf/.bashrc C:/
  mcopy -Do -s $SRCDIR/../images/i686-elf/* C:/

  mcopy -Do $SRCDIR/../scripts/termcap C:/etc

  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/apps/$BINARY ]; then
      mcopy -Do $SRCDIR/apps/$BINARY C:/applications
    fi
  done

  mcopy -Do $SRCDIR/libc.so C:/libraries
  mcopy -Do $SRCDIR/libm.so C:/libraries

  echo Only creating FAT disk image as \`losetup\' was not found.
else
  echo Not creating disk images because neither \`losetup\' nor \`mtools\' could not be found.
fi

# Live CD
if which mkisofs > /dev/null 2>&1; then
    rm -f pedigree.iso
    touch .pedigree-root
    cp $SRCDIR/../images/grub/stage2_eltorito .

    mkisofs -D -joliet -graft-points -quiet -input-charset ascii -R \
	-b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
	-boot-info-table -o pedigree.iso -V "PEDIGREE" \
	boot/grub/stage2_eltorito=./stage2_eltorito \
	boot/grub/menu.lst=$SRCDIR/../images/grub/menu.lst \
	boot/kernel=$SRCDIR/kernel/kernel \
	boot/initrd.tar=$SRCDIR/initrd.tar \
	/=$SRCDIR/../images/i686-elf/ \
	/.pedigree-root=./.pedigree-root \
	/libraries/libc.so=$SRCDIR/libc.so \
	/libraries/libm.so=$SRCDIR/libm.so \
	/applications/login=$SRCDIR/apps/login \
	/applications/TUI=$SRCDIR/apps/TUI

    rm stage2_eltorito
    rm .pedigree-root
else
    echo Not creating bootable ISO because \`mkisofs\' could not be found.
fi
