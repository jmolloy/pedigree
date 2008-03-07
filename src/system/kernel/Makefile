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
              core/processor/x86/asm \
              core/processor/x64 \
              core/processor/x64/asm \
              core/processor/x86_common \
              core/processor/mips32 \
              core/processor/mips64 \
              core/processor/mips_common \
              debugger/ \
              debugger/commands \
              debugger/libudis86 \
              linker/ \
              machine/ \
              machine/x86_common \
              machine/mips_common \
              machine/malta \
              machine/malta/bonito64 \
              machine/au1500 \
              machine/au1500/db1500 \
              utilities

###################################################################
# Source file list.
ifdef KERNEL_STANDALONE
SRCFILES = boot-standalone.s
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
            debugger/commands/BreakpointCommand.cc \
            debugger/commands/DumpCommand.cc \
            debugger/commands/StepCommand.cc \
            debugger/commands/TraceCommand.cc \
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

SRCFILES += utilities/String.cc utilities/List.cc utilities/Vector.cc
SRCFILES += Log.cc BootstrapInfo.cc
SRCFILES += core/processor/io.c
SRCFILES += machine/Machine.cc

###################################################################
# x86_commmon source file list.
ifdef X86_COMMON
SRCFILES += machine/x86_common/Apic.cc \
            machine/x86_common/Pic.cc \
            machine/x86_common/Pit.cc \
            machine/x86_common/Rtc.cc \
            machine/x86_common/Serial.cc \
            machine/x86_common/Vga.cc \
            machine/x86_common/Pc.cc \
            machine/x86_common/Machine.cc \
            core/processor/x86_common/Processor.cc
endif

###################################################################
# x86 source file list.
ifdef X86
SRCFILES += core/processor/x86/initialiseProcessor1.cc \
            core/processor/x64/initialiseProcessor2.cc \
            core/processor/x86/InterruptManager.cc \
            core/processor/x86/asm/InterruptManager.s \
            core/processor/x86/asm/Processor.s \
            core/processor/x86/Processor.cc \
            core/processor/x86/gdt.cc \
            core/processor/x86/state.cc
endif

###################################################################
# x64 source file list.
ifdef X86_64
SRCFILES += core/processor/x64/initialiseProcessor1.cc \
            core/processor/x64/initialiseProcessor2.cc \
            core/processor/x64/InterruptManager.cc \
            core/processor/x64/SyscallManager.cc \
            core/processor/x64/asm/SyscallManager.s \
            core/processor/x64/asm/InterruptManager.s \
            core/processor/x64/asm/Processor.s \
            core/processor/x64/Processor.cc \
            core/processor/x64/gdt.cc \
            core/processor/x64/state.cc
endif

###################################################################
# mips common source list.
ifdef MIPS_COMMON
SRCFILES += boot-mips.cc
endif

ifdef MIPS_MALTA
SRCFILES += machine/malta/Serial.cc
endif

ifdef MIPS_MALTA_BONITO64
SRCFILES += machine/malta/bonito64/Bonito64.cc
endif

ifdef MIPS_AU1500
# SRCFILES += ...
endif

ifdef MIPS_AU1500_DB1500
SRCFILES += machine/au1500/db1500/Db1500.cc
endif

###################################################################
# mips32 source file list.
ifdef MIPS32
SRCFILES += core/processor/mips32/initialiseProcessor1.cc \
            core/processor/mips32/initialiseProcessor2.cc
endif

###################################################################
# mips64 source file list.
ifdef MIPS64
SRCFILES += core/processor/mips64/initialiseProcessor1.cc \
            core/processor/mips64/initialiseProcessor2.cc
endif

###################################################################
# Set up environment for buildCommon.mk and call.

ifdef DEBUGGER
CMD_FLAGS += -D__UD_STANDALONE__
endif

ifdef KERNEL_STANDALONE
LDFLAGS += -T $(shell pwd)/link-standalone.ld
endif
ifdef MIPS_COMMON
LDFLAGS += -mxgot -T $(shell pwd)/link-mips.ld
endif
LDADD += -lgcc
BUILD_NAME=kernel

include $(BUILD)/buildCommon.mk

