#!/bin/bash

out=$1
bits=$2

if [ "x$out$bits" = "x" ]; then
    echo "usage: create_arm_loader.sh output_image bits_dir"
    exit 1
fi

if [ ! -e "$out" ]; then
    dd if=/dev/zero of="$out" bs=65536 count=30517 >/dev/null 2>&1
fi

fat_part="n\np\n1\n\n+50\nt\nc\na\n1\n"
ext_part="n\np\n2\n\n\n"

echo -e "o\np\n${fat_part}${ext_part}p\nw" | \
    fdisk -c=dos -u=cylinders -H 255 -S 63 "$out" >/dev/null 2>&1

sudo kpartx -a "$out"

sudo mkfs.msdos -F 32 /dev/mapper/loop0p1
sudo mkfs.ext2 /dev/mapper/loop0p2

mkdir -p img
sudo mount -t vfat /dev/mapper/loop0p1 img

sudo cp "$bits"/MLO ./img/
sudo cp "$bits"/u-boot.img ./img/u-boot.img
sudo cp ./build/uImage.bin ./img/uImage

cat <<EOF >"$bits"/pedigree.txt
setenv initrd_high "0xffffffff"
setenv fdt_high "0xffffffff"
setenv bootcmd "fatload mmc 0 88000000 uImage;bootm 88000000"
boot
EOF

sudo mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
    -n 'Booting Pedigree...' -d "$bits"/pedigree.txt ./img/boot.scr

rm "$bits"/pedigree.txt

sudo umount img
sudo kpartx -d "$out"
rm -rf img

echo "$out is ready for booting."
