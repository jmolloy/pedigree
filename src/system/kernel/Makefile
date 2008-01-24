# \author James Molloy
# \date   21-Jan-08

###################################################################
# Subdirectory list.
DIRECTORIES =

###################################################################
# Source file list.
ifdef KERNEL_STANDALONE
SRCFILES = boot-standalone.s
endif
SRCFILES += main.cc

###################################################################
# Set up environment for buildCommon.mk and call.

LDFLAGS += -T $(shell pwd)/link-standalone.ld
BUILD_NAME=kernel

include $(BUILD)/buildCommon.mk

