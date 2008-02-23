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
INCLUDES += -I./ \
            -I./../common/ \
            -I./../include/ \
            -I./debugger/ \
            -I./debugger/commands \
            -I./debugger/libudis86 \
            -I./core/lib \
            -I./linker

###################################################################
# Subdirectory list.
DIRECTORIES = core/lib \
              core/memory \
              core/process \
              core/processor/ \
              core/processor/x86 \
              core/processor/x64 \
              core/processor/x86_common \
              debugger/ \
              debugger/commands \
              debugger/libudis86 \
              linker/ \
              machine/ \
              machine/x86_common

###################################################################
# Source file list.
ifdef KERNEL_STANDALONE
SRCFILES = boot-standalone.s BootstrapInfo.cc Log.cc
endif
ifdef DEBUGGER
SRCFILES += debugger/libudis86/mnemonics.c \
            debugger/libudis86/opcmap.c \
            debugger/libudis86/input.c \
            debugger/libudis86/decode.c \
            debugger/libudis86/syn.c \
            debugger/libudis86/syn-intel.c \
            debugger/libudis86/syn-att.c \
            debugger/libudis86/udis86.c \
            debugger/commands/DisassembleCommand.cc \
            debugger/commands/QuitCommand.cc \
            debugger/commands/LogViewer.cc \
            debugger/commands/Backtracer.cc \
            debugger/LocalIO.cc \
            debugger/Debugger.cc \
            debugger/Backtrace.cc \
            debugger/StackFrame.cc
endif

SRCFILES += core/lib/cppsupport.cc \
            core/lib/demangle.cc \
            core/lib/memory.c \
            core/lib/string.c \
            core/lib/vsprintf.c \
            core/main.cc

SRCFILES += linker/Elf32.cc

SRCFILES += machine/io.cc

###################################################################
# x86_commmon source file list.
ifdef X86_COMMON
SRCFILES += machine/x86_common/initialiseMachine1.cc \
            machine/x86_common/initialiseMachine2.cc \
            core/processor/x86_common/processor.cc
endif

###################################################################
# x86 source file list.
ifdef X86
SRCFILES += core/processor/x86/initialiseProcessor.cc \
            core/processor/x86/interrupt.cc \
            core/processor/x86/asm_interrupt.S \
            core/processor/x86/asm_processor.S \
            core/processor/x86/gdt.cc
endif

###################################################################
# x64 source file list.
ifdef X86_64
SRCFILES += core/processor/x64/initialiseProcessor.cc \
            core/processor/x64/interrupt.cc \
            core/processor/x64/syscall.cc \
            core/processor/x64/asm_syscall.S \
            core/processor/x64/asm_interrupt.S \
            core/processor/x64/asm_processor.S \
            core/processor/x64/gdt.cc
endif

###################################################################
# Set up environment for buildCommon.mk and call.

ifdef DEBUGGER
CMD_FLAGS += -D__UD_STANDALONE__
endif

ifdef KERNEL_STANDALONE
LDFLAGS += -T $(shell pwd)/link-standalone.ld
endif
BUILD_NAME=kernel

include $(BUILD)/buildCommon.mk

