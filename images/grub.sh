###############################################################################
##### Pedigree                                                            #####
###############################################################################
grub --batch --no-floppy <<EOT 1>/dev/null  || exit 1
device (fd0) $1
install (fd0)/boot/grub/stage1 (fd0) (fd0)/boot/grub/stage2 (fd0)/boot/grub/menu.lst
quit
EOT
