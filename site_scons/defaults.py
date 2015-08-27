'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'''

import SCons

# Default flags for Pedigree's build, per architecture

#------------------------- All Architectures -------------------------#

# Generic entry-level flags (that everyone should have)
generic_flags = ['-fno-builtin', '-nostdlib', '-ffreestanding', '-O3']
generic_cflags = ['-std=gnu99']
generic_cxxflags = ['-std=gnu++98', '-fno-exceptions', '-fno-rtti',
                    '-fno-asynchronous-unwind-tables']

# Warning flags (that force us to write betterish code)
warning_flags = ['-Wall', '-Wextra', '-Wpointer-arith', '-Wcast-align',
                 '-Wwrite-strings', '-Wno-long-long', '-Wno-variadic-macros',
                 '-Wno-unused-parameter']

# Language-specific warnings
warning_flags_c = ['-Wnested-externs']
warning_flags_cxx = []

# Turn off some warnings (because they kill the build)
warning_flags_off = ['-Wno-unused', '-Wno-unused-variable', '-Wno-conversion',
                     '-Wno-format', '-Wno-empty-body', '-Wno-packed-bitfield-compat']

# Generic assembler flags
generic_asflags = []

# Generic link flags (that everyone should have)
generic_linkflags = ['-nostdlib', '-nostartfiles']

# Generic defines
generic_defines = [
    'DEBUGGER_QWERTY',          # Enable the QWERTY keymap
    'ADDITIONAL_CHECKS',
    'VERBOSE_LINKER',           # Increases the verbosity of messages from the Elf and KernelElf classes
]

#------------------------- x86 (+x64) -------------------------#

# x86 defines - udis86 uses __UD_STANDALONE__
general_x86_defines = ['X86_COMMON', 'LITTLE_ENDIAN', '__UD_STANDALONE__', 'THREADS', 'KERNEL_STANDALONE']

#---------- x86, 32-bit ----------#

# 32-bit defines
x86_32_defines = ['X86', 'BITS_32', 'KERNEL_NEEDS_ADDRESS_SPACE_SWITCH']

# 32-bit CFLAGS and CXXFLAGS - disable SSE and MMX generation, we only use it
# in very specific implementations (eg SSE-powered memset/memcpy).
default_x86_flags = ['-march=pentium4', '-mtune=k8', '-mno-sse', '-mno-mmx']
default_x86_cflags = []
default_x86_cxxflags = []

# 32-bit assembler flags
default_x86_asflags = ['-felf32']

# 32-bit linker flags
default_x86_linkflags = ['-Tsrc/system/kernel/core/processor/x86/kernel.ld']

# x86 32-bit images directory
default_x86_imgdir = '#images/x86'

# x86 final variables
x86_flags = default_x86_flags + generic_flags + warning_flags + warning_flags_off
x86_cflags = default_x86_cflags + generic_cflags + warning_flags_c
x86_cxxflags = default_x86_cxxflags + generic_cxxflags + warning_flags_cxx
x86_asflags = generic_asflags + default_x86_asflags
x86_linkflags = generic_linkflags + default_x86_linkflags
x86_defines = generic_defines + general_x86_defines + x86_32_defines

#---------- x86_64 ----------#

# x64 defines
x64_defines = ['X64', 'BITS_64']

# x64 CFLAGS and CXXFLAGS
# x86_64 omits the frame pointer by default with optimisation enabled, which
# is great until we need to backtrace. This should probably be an option.
default_x64_flags = ['-m64', '-mno-red-zone', '-mcmodel=kernel', '-march=k8',
                     '-mno-sse', '-mno-mmx', '-fno-omit-frame-pointer']
default_x64_cflags = []
default_x64_cxxflags = []

# x64 assembler flags
default_x64_asflags = ['-felf64']

# x64 linker flags
default_x64_linkflags = ['-Tsrc/system/kernel/core/processor/x64/kernel.ld']

# x64 images directory
default_x64_imgdir = '#images/x64'

# x64 final variables
x64_flags = generic_flags + default_x64_flags + warning_flags + warning_flags_off
x64_cflags = generic_cflags + default_x64_cflags + warning_flags_c
x64_cxxflags = generic_cxxflags + default_x64_cxxflags + warning_flags_cxx
x64_asflags = generic_asflags + default_x64_asflags
x64_linkflags = generic_linkflags + default_x64_linkflags
x64_defines = generic_defines + general_x86_defines + x64_defines

#------------------------- ARM -------------------------#

# ARM defines
# TODO: Fix this to support other ARM chips and boards
general_arm_defines = ['ARM_COMMON', 'BITS_32', 'THREADS', 'STATIC_DRIVERS']

# ARM CFLAGS and CXXFLAGS
default_arm_flags = ['-fno-omit-frame-pointer', '-mabi=aapcs', '-mapcs-frame']
default_arm_cflags = []
default_arm_cxxflags = []

# ARM assembler flags
default_arm_asflags = []

# ARM linker flags - [mach] is replaced with the machine type
default_arm_linkflags = ['-Tsrc/system/kernel/link-arm-[mach].ld']

# ARM images directory
default_arm_imgdir = '#images/arm'

# arm final variables
arm_flags = generic_flags + default_arm_flags + warning_flags + warning_flags_off
arm_cflags = generic_cflags + default_arm_cflags + warning_flags_c
arm_cxxflags = generic_cxxflags + default_arm_cxxflags + warning_flags_cxx
arm_asflags = generic_asflags + default_arm_asflags
arm_linkflags = generic_linkflags + default_arm_linkflags
arm_defines = [x for x in generic_defines if x != "SERIAL_IS_FILE"] + general_arm_defines

#------------------------- Import Dictionaries -------------------------#
default_flags       = {'x86' : x86_flags, 'x64' : x64_flags , 'arm' : arm_flags}
default_cflags      = {'x86' : x86_cflags, 'x64' : x64_cflags , 'arm' : arm_cflags}
default_cxxflags    = {'x86' : x86_cxxflags, 'x64' : x64_cxxflags, 'arm' : arm_cxxflags }
default_asflags     = {'x86' : x86_asflags, 'x64' : x64_asflags, 'arm' : arm_asflags }
default_linkflags   = {'x86' : x86_linkflags, 'x64' : x64_linkflags, 'arm' : arm_linkflags }
default_imgdir      = {'x86' : default_x86_imgdir, 'x64' : default_x64_imgdir, 'arm' : default_arm_imgdir }
default_defines     = {'x86' : x86_defines, 'x64' : x64_defines, 'arm' : arm_defines }
