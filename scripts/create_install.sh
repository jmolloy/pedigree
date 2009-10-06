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

SRCDIR=../build

# Hard disk files
HDFILES="login TUI"

# Create a hard disk image for use as a ramdisk
if which mcopy >/dev/null 2>&1; then

  # disk.img, not hdd_fat16.img
  rm -f disk.img
  mkdir -p $SRCDIR/tmp
  cd $SRCDIR/tmp
  tar -xzf ../$SRCDIR/../images/hdd_fat16.tar.gz
  mv hdd_fat16.img ../disk.img
  cd ..
  rm -rf tmp

  sh $SRCDIR/../scripts/mtsetup.sh ./disk.img > /dev/null 2>&1

  # Set the volume label to the installer image label
  mlabel -c C:INSRAMFS

  # This should be the root disk
  touch ./.pedigree-root
  mcopy -Do ./.pedigree-root C:/
  rm ./.pedigree-root

  mcopy -Do -s $SRCDIR/../images/install/disk-contents/* C:/

  # Copy libc/libm shared objects
  mcopy -Do $SRCDIR/libc.so C:/libraries
  mcopy -Do $SRCDIR/libm.so C:/libraries

  # Copy across relevant binaries
  for f in $HDFILES; do
    BINARY=`echo $f | sed 's,.*/\([^/]*\)$,\1,'`
    if [ -f $SRCDIR/apps/$BINARY ]; then
      mcopy -Do $SRCDIR/apps/$BINARY C:/applications
    fi
  done

  echo Install hard disk image created
fi

# Redirect to the CD script
cd ../images/install
./create_install.sh
cd ../../build

# Remove the ramdisk now
rm -f $SRCDIR/disk.img
