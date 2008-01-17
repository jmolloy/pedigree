
ifndef LOUD
SILENTFLAG := -s
endif

COMMAND_LINE_FLAGS = $(MAKEFLAGS)
SRC = ../src
INCLUDES=

# Cos eclipse is stupid.
PATH += :/usr/jimix/bin
export PATH

export COMMAND_LINE_FLAGS
export SILENTFLAG
export SRC
export INCLUDES

all: x86

x86:
	$(MAKE) $(SILENTFLAG) -f Makefile.x86

clean:
	-@rm -Rf objects-jimix
	-@rm -Rf objects-lib
	-@rm libc.a libm.a libg.a kernel
	-cd ../initrd/ && $(MAKE) clean

bochs: x86
	cd ../../scripts && \
	./update_image.sh ../jimix/build/kernel ../jimix/initrd/initrd.img ../images/floppy.img && \
	./run_bochs.sh ../images/floppy.img

qemu: all
	cd ../../scripts && \
	./update_image.sh ../jimix/build/kernel ../jimix/initrd/initrd.img ../images/floppy.img && \
	./run_qemu.sh ../images/floppy.img ../images/hdd_16h_63spt_100c
