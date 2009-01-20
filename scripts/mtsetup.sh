#!/bin/bash
# Copyright Anthony Richardson, published without license on the mtools mailing list
# (http://lll.lgl.lu/pipermail/mtools/2004/000062.html)
# usage: mtsetup [-f | -h | --help] image [part #]

error() {
  echo $*
}

if [ $# -lt 1 -o $# -gt 3 -o "$1" = "--help" ]
then
   error "usage: mtsetup [-f | -h | --help] image [part #]"
   error ""
   error "Configures mtools so that image is the current mtools drive"
   error "At any one time you may have both an active floppy drive (A:)"
   error "and an active hard drive (C:)."
   error
   error "If a partition # is specified (1-4) then a hard drive image
is"
   error "assumed.  The -f and -h options can be used to indicate"
   error "whether the image is a floppy or hard drive image. If these"
   error "options are used the filesystem is not checked.  This is"
   error "useful if the filesystem is to be formatted with mformat."
   error "If -f or -h are not used mtsetup tries to deduce whether the"
   error "image is a floppy or hard drive image (with one partition)."
   error
   error "In practice, you just need to type \"mtsetup floppya.img\" to"
   error "access the image file as drive A.  If you then type,"
   error "\"mtsetup harddisk.img\" then partition 1 of this disk will"
   error "available as drive C: (and the last floppy image will still"
   error "be available as A:)"
   exit 1
fi

forceflop=false
forcehard=false
if [ "$1" = "-f" ]
then
   forceflop=true
   shift
fi
if [ "$1" = "-h" ]
then
   forcehard=true
   shift
fi

# Default MTSETUP .mtoolsrc file
line1="#MTSETUP"
line2="MTOOLS_NO_VFAT=1"
line3="drive A: file=\"\" 1.44m mformat_only"
line4="drive C: file=\"\" partition=1"

mtsrc=$HOME/.mtoolsrc

gen_mtsrc () {
   echo "#MTSETUP" > $mtsrc
   echo "MTOOLS_NO_VFAT=1" >> $mtsrc
   echo "drive A: file=\"$1\" 1.44m mformat_only" >> $mtsrc
   echo "drive C: file=\"$2\" partition=$3" >> $mtsrc
}

# See if $mtsrc exists
if [ -s $mtsrc ]
then
   # Yes, it does.
   # Read first line and check if it is an MTSETUP file
   read < $mtsrc
   if [ "$REPLY" != "#MTSETUP" ]
   then
      # Not an MTSETUP file, overwrite it
      gen_mtsrc "" "" "1"
   fi
else
   # $mtsrc does not exist, create a default one
   gen_mtsrc "" "" "1"
fi

# OK, now we should have a standard MTSETUP file
# Read it in
{ read line1; read line2; read line3; read line4; } < $mtsrc

flopfile=$(echo "$line3" | cut -d' ' -f3 | cut -d\" -f2)
hardfile=$(echo "$line4" | cut -d' ' -f3 | cut -d\" -f2)
partition=$(echo "$line4" | cut -d' ' -f4 | cut -d= -f2)

hardimg=true
if [ $forcehard = true -o $# = 2 ]
then
   # A hard drive image
   if [ $# = 1 ]
   then
      # no hard drive partition given
      set "$1" "1"
   fi
   gen_mtsrc "$flopfile" "$1" "$2"
   if [ $forcehard = false ]
   then
      if ! mcd C: 2> /dev/null
      then
         error "Partition $2 of image $1 does not appear to be a FAT"
         error "formatted file system. (Use -h to force.)"
         gen_mtsrc "$flopfile" "$hardfile" "$partition"
         exit 1
      fi
   fi
elif [ $forceflop = true ]
then
   # A floppy drive image
   gen_mtsrc "$1" "$hardfile" "$partition"
   hardimg=false
else
   # Image type unknown
   # guess hard drive partition
   gen_mtsrc "$flopfile" "$1" "1"
   if ! mcd C: 2> /dev/null
   then
      # not a hard drive image, try a floppy image
      gen_mtsrc "$1" "$hardfile" "$partition"
      if ! mcd A: 2> /dev/null
      then
         error "Hmmm. Does not appear to be a standard floppy or"
         error "hard disk with FAT on partition 1. Giving up."
         error "Use -f or -h option to force."
         gen_mtsrc "$flopfile" "$hardfile" "$partition"
         exit 1
      fi
      hardimg=false
   else
      partition=1
   fi
fi

if [ $hardimg = true ]
then
   echo "Partition $partition of hard disk image $hardfile is MTOOLS
drive C:"
else
   echo "Floppy disk image $flopfile is MTOOLS drive A:"
fi
