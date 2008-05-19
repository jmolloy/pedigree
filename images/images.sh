###############################################################################
##### Pedigree                                                            #####
###############################################################################

# Create directory fd
mkdir fd
chmod 0777 fd

# Create empty images
dd if=/dev/zero of=images/floppy_ext2.img bs=1024 count=1440
dd if=/dev/zero of=images/floppy_fat.img bs=1024 count=1440

# Format the images
mke2fs -F images/floppy_ext2.img
mkdosfs images/floppy_fat.img

# Mount the Ext2 image
sudo mount -oloop images/floppy_ext2.img fd
mkdir fd/boot
mkdir fd/boot/grub
cp compilers/dir/lib/grub/x86_64-unknown/stage1 fd/boot/grub
cp compilers/dir/lib/grub/x86_64-unknown/stage2 fd/boot/grub
cp images/menu.lst fd/boot/grub
sudo umount fd

# Mount the Fat image
sudo mount -oloop images/floppy_fat.img fd
sudo mkdir fd/boot
sudo mkdir fd/boot/grub
sudo cp compilers/dir/lib/grub/x86_64-unknown/stage1 fd/boot/grub
sudo cp compilers/dir/lib/grub/x86_64-unknown/stage2 fd/boot/grub
sudo cp images/menu.lst fd/boot/grub
sudo umount fd

# Install grub
sh images/grub.sh images/floppy_ext2.img
sh images/grub.sh images/floppy_fat.img

# Delete directory fd
rmdir fd

# Create an ISO image
mkdir iso
mkdir iso/boot
mkdir iso/boot/grub
cp compilers/dir/lib/grub/x86_64-unknown/stage2_eltorito iso/boot/grub
cp images/menu.lst iso/boot/grub/menu.lst
mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o images/pedigree.iso iso
rm -rf iso
