#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

###################################################################
# Extra includes.
INCLUDES += -I . \
            -I./debugger/ \
            -I./debugger/commands \
            -I./debugger/libudis86 \
            -I./utility/ \

###################################################################
# Subdirectory list.
DIRECTORIES = debugger/ \
              debugger/commands \
              debugger/libudis86 \
              utility/ \
              machine/ machine/x86 machine/x86_64 machine/x86_common

###################################################################
# Source file list.
ifdef KERNEL_STANDALONE
SRCFILES = boot-standalone.s
endif
ifdef DEBUGGER
SRCFILES += debugger/libudis86/mnemonics.c debugger/libudis86/opcmap.c debugger/libudis86/input.c debugger/libudis86/decode.c \
            debugger/libudis86/syn.c debugger/libudis86/syn-intel.c debugger/libudis86/syn-att.c debugger/libudis86/udis86.c \
            debugger/LocalIO.cc debugger/Debugger.cc debugger/commands/DisassembleCommand.cc
endif
SRCFILES += machine/io.cc
ifdef X86
SRCFILES += machine/x86_common/initialiseMachine1.cc machine/x86_common/initialiseMachine2.cc
endif
ifdef X86_64
SRCFILES += machine/x86_common/initialiseMachine1.cc machine/x86_common/initialiseMachine2.cc
endif
SRCFILES += utility/string.c utility/memory.c utility/vsprintf.c
SRCFILES += cppsupport.cc main.cc

###################################################################
# Set up environment for buildCommon.mk and call.

ifdef DEBUGGER
CMD_FLAGS += -D__UD_STANDALONE__
endif

LDFLAGS += -T $(shell pwd)/link-standalone.ld
BUILD_NAME=kernel

include $(BUILD)/buildCommon.mk

