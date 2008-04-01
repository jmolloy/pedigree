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
ifdef X64
COMPILER=amd64-elf
endif
ifdef MIPS64
COMPILER=mips64el-elf
endif
ifdef MIPS32
COMPILER=mips64el-elf
endif

ifdef X86
CMD_FLAGS += -DX86_COMMON
export X86_COMMON=1
endif
ifdef X64
CMD_FLAGS += -DX86_COMMON
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
ifdef X86_COMMON
AS=nasm
endif
ifdef MIPS_COMMON
AS=$(BUILD)/compilers/$(COMPILER)/bin/as
endif
CFLAGS=-fno-builtin -fno-stack-protector -nostdlib
ifdef X86
  CFLAGS += -m32  -gdwarf-2 -g1
endif
ifdef X64
  CFLAGS += -fno-omit-frame-pointer -m64 -mno-red-zone -mno-sse -mcmodel=kernel  -gdwarf-2 -g1
endif
CXXFLAGS= -Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib  -gdwarf-2 -g1
ifdef X86
  CXXFLAGS += -m32  -gdwarf-2 -g1
endif
ifdef X64
  CXXFLAGS += -fno-omit-frame-pointer -m64 -mno-red-zone -mno-sse -mcmodel=kernel  -gdwarf-2 -g1
endif
ifdef MIPS32
  CXXFLAGS +=  -G0 -gdwarf-2 -g1 -mips32 -mabi=32 -mxgot -static -fomit-frame-pointer -gdwarf-2 -g1
  CFLAGS +=  -G0 -gdwarf-2 -g1 -mips32 -mabi=32 -mxgot -static -fomit-frame-pointer -gdwarf-2 -g1
  ASFLAGS +=  -G0 -gdwarf-2 -g1 -mips32 -mabi=32 -gdwarf-2 -g1
  LDFLAGS += -G0
endif
ifdef MIPS64
  CXXFLAGS += -g -gdwarf-2 -g1 -mips64 -mabi=64 -mxgot -static -fomit-frame-pointer -fasynchronous-unwind-tables
  CFLAGS += -g -gdwarf-2 -g1 -mips64 -mabi=64 -mxgot -static -fomit-frame-pointer -fasynchronous-unwind-tables
endif
LDFLAGS +=-nostdlib -nostdinc
ifdef X86
  ASFLAGS=-felf
endif
ifdef X64
  ASFLAGS=-felf64
endif

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

# Check for cygwin-ness.
ifeq ($(TERM),cygwin)
CYGWIN := 1
endif

# Add compilers/bin to our path so we can pick up our custom compilers.
PATH := $(PATH):$(BUILD)/compilers/bin

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
export X64                  # Was X64 specified on the cmdline?
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

common: compiler
	@cd $(SRC)/system/common; $(MAKE) $(SILENTFLAG)

kernel: compiler common
	@cd $(SRC)/system/kernel; $(MAKE) $(SILENTFLAG)

modules: compiler common # For every module in $MODULES, export the module name and call the module makefile.
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
	-@rm -f built/*

image: kernel
	@perl -e "print \"\e[32m*** Writing disk images...\e[0m\n\";" # echo doesn't have support for colours.
ifdef X86_COMMON
ifdef CYGWIN
	@cp images/floppy_fat.img built/floppy.img
	@mcopy -Do -i  built/floppy.img built/kernel ::/
else
	-@mkdir -p /tmp/pedigree-image
	@cp images/floppy_ext2.img built/floppy.img
	@sudo /sbin/losetup /dev/loop0 built/floppy.img
	@sudo mount -o loop /dev/loop0 /tmp/pedigree-image
	@sudo cp built/kernel /tmp/pedigree-image/kernel
	@sudo umount /dev/loop0
	@sudo /sbin/losetup -d /dev/loop0
endif
endif
ifdef MIPS_COMMON
	@$(BUILD)/compilers/$(COMPILER)/bin/objcopy built/kernel -O binary built/kernel.bin
	@$(BUILD)/compilers/$(COMPILER)/bin/objcopy built/kernel -O srec built/kernel.srec
	@$(BUILD)/compilers/$(COMPILER)/bin/objcopy built/bootloader -O srec built/bootloader.srec
endif
	@perl -e "print \"\e[32m*** Build finished.\e[0m\n\";" # echo doesn't have support for colours.

