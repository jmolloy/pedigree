#!/bin/bash

out=$1
uboot=$2
build_path=$3

mkdir -p build/bits
bits="build/bits"

if [ "x$out$uboot$build_path" = "x" ]; then
    echo "usage: create_arm_loader.sh output_image uboot_dir"
    exit 1
fi

CYLINDER_SIZE=8225280

if [ ! -e "A$bits/out.img" ]; then
    dd if=/dev/zero of="$bits/out.img" bs=$CYLINDER_SIZE count=100
fi

fat_part="n\np\n1\n\n+50\nt\nc\na\n1\n"
ext_part="n\np\n2\n\n\n"

# Partition main disk image.
echo -e "o\np\n${fat_part}${ext_part}p\nw" | \
    fdisk -c=dos -u=cylinders -H 255 -S 63 "$bits/out.img"

# Create new filesystems ready for access.
dd if=/dev/zero of="$bits/fat.img" bs=$CYLINDER_SIZE count=50
dd if=/dev/zero of="$bits/ext2.img" bs=$CYLINDER_SIZE count=49

# Create ext2 filesystem.
mkfs.ext2 -F "$bits/ext2.img"

# Pedigree boot script.
cat <<EOF >"$bits/pedigree.txt"
setenv initrd_high "0xffffffff"
setenv fdt_high "0xffffffff"
setenv bootcmd "fatload mmc 0 88000000 uImage;bootm 88000000"
boot
EOF

mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n 'Booting Pedigree...' \
    -d "$bits/pedigree.txt" "$bits/boot.scr"

# Create FAT filesystem.
cat <<EOF >.mtoolsrc
drive z: file="$bits/out.img" fat_bits=32 cylinders=50 heads=255 sectors=63 partition=1 mformat_only
EOF

export MTOOLSRC="$PWD/.mtoolsrc"

# Copy files into the FAT partition.
mformat z:
mcopy "$uboot/MLO" "$uboot/u-boot.img" "$bits/boot.scr" z:
mcopy "$build_path/uImage.bin" z:uImage

rm -f .mtoolsrc

mv "$bits/out.img" "$out"

echo "$out is ready for booting."
