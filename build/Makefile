
ifndef LOUD
SILENTFLAG := -s
endif

COMMAND_LINE_FLAGS = $(MAKEFLAGS)
SRC = ..
INCLUDES=

include $(SRC)/modules/directory.mk

CC=i686-elf-cc
CXX=i686-elf-g++
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

all: modules

bootstrap:
	$(MAKE) $(SILENTFLAG) -f Makefile.bootstrap

kernel:
	$(MAKE) $(SILENTFLAG) -f Makefile.kernel

modules:
	@for NAME in ${MODULES} ; \
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

