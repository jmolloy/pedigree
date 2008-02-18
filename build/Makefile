#########################################################################################
# \author James Molloy
# \date   18-Jan-2008

# If we weren't told to be verbose, set SILENTFLAG to -s so make surpresses some output.
ifndef LOUD
SILENTFLAG := -s
endif

COMMAND_LINE_FLAGS = $(MAKEFLAGS)
SRC = ../src
INCLUDES=
CMD_FLAGS1 = $(patsubst -%,, $(COMMAND_LINE_FLAGS))
CMD_FLAGS = $(patsubst %,-D%, $(CMD_FLAGS1))

# /build/buildFlags.mk contains a list of standard build flags.
include buildFlags.mk

# /modules/modules.list contains a definition for $MODULES, with the name of all modules to make.
include $(SRC)/system/modules/modules.list

COMPILER=
ifdef X86
COMPILER=i686-elf
endif
ifdef X86_64
COMPILER=amd64-elf
endif

ifdef X86
CMD_FLAGS+= -DX86_COMMON
export X86_COMMON=1
endif
ifdef X86_64
CMD_FLAGS+= -DX86_COMMON
export X86_COMMON=1
endif

BUILD=$(shell pwd)
ifdef TERMINAL_DUMB
CC=$(COMPILER)-gcc
CXX=$(COMPILER)-g++
else
CC=$(BUILD)/gcc
CXX=$(BUILD)/g++
endif
CPP=$(COMPILER)-cpp
AS=nasm
CFLAGS=-fno-builtin -fno-stack-protector -m32 -nostdlib
CXXFLAGS=-fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -m32 -nostdlib -O0
LDFLAGS=-nostdlib -nostdinc
ASFLAGS=-felf
ifdef OPTIMIZE_1
CFLAGS+=-O1
CXXFLAGS+=-O1
endif
ifdef OPTIMIZE_2
CFLAGS+=-O2
CXXFLAGS+=-O2
endif
ifdef OPTIMIZE_3
CFLAGS+=-O3
CXXFLAGS+=-O3
endif
ifdef OPTIMIZE_S
CFLAGS+=-Os
CXXFLAGS+=-Os
endif

# Add compilers/bin to our path so we can pick up our custom compilers.
PATH:=$(PATH):$(BUILD)/compilers/bin

export CMD_FLAGS
export SILENTFLAG
export SRC
export INCLUDES
export CC
export CXX
export CPP
export AS
export CFLAGS
export CXXFLAGS
export LDFLAGS
export ASFLAGS
export BUILD
export X86                  # Was X86 specified on the cmdline?
export X86_64               # Was X86_64 specified on the cmdline?
export KERNEL_STANDALONE    # Was KERNEL_STANDALONE specified on the cmdline?
export COMPILER
export PATH

all: kernel bootstrap modules drivers user subsys image

buildFlags.mk:
	@perl ./buildFlags.pl

compiler:
	@perl ./checkBuildSystem.pl $(COMPILER)

bootstrap: compiler
	@cd $(SRC)/system/boot; $(MAKE) $(SILENTFLAG)

utilities: compiler
	@cd $(SRC)/system/utilities; $(MAKE) $(SILENTFLAG)

kernel: compiler utilities
	@cd $(SRC)/system/kernel; $(MAKE) $(SILENTFLAG)

modules: compiler utilities # For every module in $MODULES, export the module name and call the module makefile.
	@STARTDIR=`pwd`; \
	for NAME in ${MODULES}; \
	do export MODULE=$$NAME; \
	cd $$STARTDIR; \
	cd $(SRC)/system/modules/$$NAME; $(MAKE) $(SILENTFLAG); \
	done

drivers: compiler kernel modules
	@cd $(SRC)/drivers; $(MAKE) $(SILENTFLAG)

user: compiler kernel modules drivers subsys
	@cd $(SRC)/user; $(MAKE) $(SILENTFLAG)

subsys: compiler kernel modules drivers
	@cd $(SRC)/subsys; $(MAKE) $(SILENTFLAG)

clean:
	-@rm buildFlags.mk
	-@rm -Rf objects-*
	-@rm built/*

image: kernel
	@perl -e "print \"\e[32m*** Writing disk images...\e[0m\n\";" # echo doesn't have support for colours.
	-@mkdir -p /tmp/pedigree-image
	@cp images/floppy.img built/
	@sudo /sbin/losetup /dev/loop0 built/floppy.img
	@sudo mount -o loop /dev/loop0 /tmp/pedigree-image
	-@sudo rm /tmp/pedigree-image/kernel
	@sudo cp built/kernel /tmp/pedigree-image/kernel
	@sudo umount /dev/loop0
	@sudo /sbin/losetup -d /dev/loop0
	@perl -e "print \"\e[32m*** Build finished.\e[0m\n\";" # echo doesn't have support for colours.

