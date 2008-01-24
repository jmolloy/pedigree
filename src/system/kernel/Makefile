# \author James Molloy
# \date   21-Jan-08

###################################################################
# Extra includes.
INCLUDES += -I./debugger/ \
						-I./debugger/libudis86

###################################################################
# Subdirectory list.
DIRECTORIES = debugger/ \
							debugger/libudis86 \
							utility/

###################################################################
# Source file list.
ifdef KERNEL_STANDALONE
SRCFILES = boot-standalone.s
endif
ifdef DEBUGGER
SRCFILES += debugger/libudis86/mnemonics.c debugger/libudis86/opcmap.c debugger/libudis86/input.c debugger/libudis86/decode.c \
            debugger/libudis86/syn.c debugger/libudis86/syn-intel.c debugger/libudis86/syn-att.c debugger/libudis86/udis86.c
endif
SRCFILES += utility/string.c utility/memory.c utility/vsprintf.c
SRCFILES += main.cc

###################################################################
# Set up environment for buildCommon.mk and call.

ifdef DEBUGGER
CMD_FLAGS += -D__UD_STANDALONE__
endif

LDFLAGS += -T $(shell pwd)/link-standalone.ld
BUILD_NAME=kernel

include $(BUILD)/buildCommon.mk

