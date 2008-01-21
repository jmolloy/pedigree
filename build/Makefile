# \author James Molloy
# \date   18-Jan-2008

# If we weren't told to be verbose, set SILENTFLAG to -s so make surpresses some output.
ifndef LOUD
SILENTFLAG := -s
endif

COMMAND_LINE_FLAGS = $(MAKEFLAGS)
SRC = ../src
INCLUDES=

# /modules/modules.list contains a definition for $MODULES, with the name of all modules to make.
include $(SRC)/system/modules/modules.list

BUILD=$(shell pwd)
CC=$(BUILD)/gcc
CXX=$(BUILD)/g++
CPP=i686-elf-cpp
AS=nasm
CFLAGS=-fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -m32 -nostdlib
CXXFLAGS=-fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -m32 -nostdlib -O0
LDFLAGS=
ASFLAGS=-felf

export COMMAND_LINE_FLAGS
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

all: kernel bootstrap modules drivers user subsys

bootstrap:
	@cd $(SRC)/system/boot; $(MAKE) $(SILENTFLAG)

utilities:
	@cd $(SRC)/system/utilities; $(MAKE) $(SILENTFLAG)

kernel: utilities
	@cd $(SRC)/system/kernel; $(MAKE) $(SILENTFLAG)

modules: utilities # For every module in $MODULES, export the module name and call the module makefile.
	@STARTDIR=`pwd`; \
	for NAME in ${MODULES}; \
	do export MODULE=$$NAME; \
	cd $$STARTDIR; \
	cd $(SRC)/system/modules/$$NAME; $(MAKE) $(SILENTFLAG); \
	done

drivers: kernel modules
	@cd $(SRC)/drivers; $(MAKE) $(SILENTFLAG)

user: kernel modules drivers subsys
	@cd $(SRC)/user; $(MAKE) $(SILENTFLAG)

subsys: kernel modules drivers
	@cd $(SRC)/subsys; $(MAKE) $(SILENTFLAG)

clean:
	-@rm -Rf objects-*
	-@rm kernel bootstrap
	-@for NAME in ${MODULES} ; \
	do rm $$NAME; done
