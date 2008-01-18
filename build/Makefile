# \author James Molloy
# \date   18-Jan-2008

# If we weren't told to be verbose, set SILENTFLAG to -s so make surpresses some output.
ifndef LOUD
SILENTFLAG := -s
endif

COMMAND_LINE_FLAGS = $(MAKEFLAGS)
SRC = ..
INCLUDES=

# /modules/directory.mk contains a definition for $MODULES, with the name of all modules to make.
include $(SRC)/modules/directory.mk

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

all: kernel bootstrap modules

bootstrap:
	$(MAKE) $(SILENTFLAG) -f Makefile.bootstrap

kernel:
	$(MAKE) $(SILENTFLAG) -f Makefile.kernel

modules: # For every module in $MODULES, export the module name and call the module makefile.
	@for NAME in ${MODULES}; \
	do export MODULE=$$NAME; \
	$(MAKE) $(SILENTFLAG) -f Makefile.modules; \
	done

clean:
	-@rm -Rf objects-kernel
	-@rm -Rf objects-bootstrap
	-@rm kernel bootstrap
	-@for NAME in ${MODULES} ; \
	do rm -Rf objects-modules-$$NAME;\
	rm $$NAME; \
	done

