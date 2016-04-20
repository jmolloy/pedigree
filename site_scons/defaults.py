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
generic_cxxflags = ['-std=gnu++11', '-fno-exceptions', '-fno-rtti',
                    '-fno-asynchronous-unwind-tables']

# Warning flags (that force us to write betterish code)
# , '-pedantic'
warning_flags = [
    '-Wall', '-Wextra', '-Wpointer-arith', '-Wcast-align',
    '-Wwrite-strings', '-Wno-long-long', '-Wvariadic-macros',
    '-Wno-unused-parameter', '-Wuninitialized', '-Wstrict-aliasing',
    '-Wsuggest-attribute=noreturn', '-Wtrampolines', '-Wfloat-equal',
    '-Wundef', '-Wcast-qual', '-Wlogical-op', '-Wdisabled-optimization']

# Language-specific warnings
warning_flags_c = ['-Wnested-externs', '-Wbad-function-cast']
# Note: Enabling a subset of warnings from -Weffc++
warning_flags_cxx = ['-Wsign-promo', '-Woverloaded-virtual',
                     '-Wnon-virtual-dtor', '-Wctor-dtor-privacy', '-Wabi',
                     '-Wuseless-cast']

# Turn off some warnings (because they kill the build)
# TODO(miselin): we really shouldn't disable -Wconversion...
warning_flags_off = [
    '-Wno-unused', '-Wno-unused-variable', '-Wno-conversion', '-Wno-format',
    '-Wno-packed-bitfield-compat', '-Wno-error=disabled-optimization',
    '-Wno-error=deprecated-declarations']

# Generic assembler flags
generic_asflags = []

# Generic link flags (that everyone should have)
generic_linkflags = ['-nostdlib', '-nostartfiles']

# Generic defines
generic_defines = [
    'DEBUGGER_QWERTY',          # Enable the QWERTY keymap
    'ADDITIONAL_CHECKS',
    # Increases the verbosity of messages from the Elf and KernelElf classes
    'VERBOSE_LINKER',
    # Make udis86 standalone (no FILE* et al) - it's linked in for all targets
    # now, instead of just x86.
    '__UD_STANDALONE__',
]

#------------------------- x86 (+x64) -------------------------#

general_x86_defines = [
    'X86_COMMON', 'LITTLE_ENDIAN', 'THREADS', 'KERNEL_STANDALONE', 'MULTIBOOT']

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
x86_flags = (
    default_x86_flags + generic_flags + warning_flags + warning_flags_off)
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
default_x64_linkflags = ['-Tsrc/system/kernel/core/processor/x64/kernel.ld',
                         '-mcmodel=kernel', '-m64']

# x64 images directory
default_x64_imgdir = '#images/x64'

# x64 final variables
x64_flags = (
    generic_flags + default_x64_flags + warning_flags + warning_flags_off)
x64_cflags = generic_cflags + default_x64_cflags + warning_flags_c
x64_cxxflags = generic_cxxflags + default_x64_cxxflags + warning_flags_cxx
x64_asflags = generic_asflags + default_x64_asflags
x64_linkflags = generic_linkflags + default_x64_linkflags
x64_defines = generic_defines + general_x86_defines + x64_defines

#------------------------- ARM -------------------------#

# ARM defines
# TODO: Fix this to support other ARM chips and boards
general_arm_defines = ['ARM_COMMON', 'BITS_32', 'THREADS', 'STATIC_DRIVERS',
                       'MULTIBOOT']

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
arm_flags = (
    generic_flags + default_arm_flags + warning_flags + warning_flags_off)
arm_cflags = generic_cflags + default_arm_cflags + warning_flags_c
arm_cxxflags = generic_cxxflags + default_arm_cxxflags + warning_flags_cxx
arm_asflags = generic_asflags + default_arm_asflags
arm_linkflags = generic_linkflags + default_arm_linkflags
arm_defines = [x for x in generic_defines if x !=
               'SERIAL_IS_FILE'] + general_arm_defines

#---------- PowerPC ----------#

# ppc defines
ppc_defines = ['PPC', 'PPC_COMMON', 'BIG_ENDIAN', 'BITS_32', 'PPC32', 'THREADS',
               'KERNEL_PROCESSOR_NO_PORT_IO', 'OMIT_FRAMEPOINTER', 'PPC_MAC',
               'OPENFIRMWARE']

# ppc CFLAGS and CXXFLAGS
default_ppc_flags = ['-fno-stack-protector', '-fomit-frame-pointer']
default_ppc_cflags = []
default_ppc_cxxflags = []

# ppc assembler flags
default_ppc_asflags = []

# ppc linker flags
default_ppc_linkflags = ['-Tsrc/system/kernel/link-ppc.ld']

# ppc images directory
default_ppc_imgdir = '#images/ppc'

# ppc final variables
ppc_flags = (
    generic_flags + default_ppc_flags + warning_flags + warning_flags_off)
ppc_cflags = generic_cflags + default_ppc_cflags + warning_flags_c
ppc_cxxflags = generic_cxxflags + default_ppc_cxxflags + warning_flags_cxx
ppc_asflags = generic_asflags + default_ppc_asflags
ppc_linkflags = generic_linkflags + default_ppc_linkflags
ppc_defines = generic_defines + ppc_defines

#------------------------- Import Dictionaries -------------------------#
default_flags = {
    'x86': x86_flags,
    'x64': x64_flags,
    'arm': arm_flags,
    'ppc': ppc_flags
}
default_cflags = {
    'x86': x86_cflags,
    'x64': x64_cflags,
    'arm': arm_cflags,
    'ppc': ppc_cflags
}
default_cxxflags = {
    'x86': x86_cxxflags,
    'x64': x64_cxxflags,
    'arm': arm_cxxflags,
    'ppc': ppc_cxxflags
}
default_asflags = {
    'x86': x86_asflags,
    'x64': x64_asflags,
    'arm': arm_asflags,
    'ppc': ppc_asflags
}
default_linkflags = {
    'x86': x86_linkflags,
    'x64': x64_linkflags,
    'arm': arm_linkflags,
    'ppc': ppc_linkflags
}
default_imgdir = {
    'x86': default_x86_imgdir,
    'x64': default_x64_imgdir,
    'arm': default_arm_imgdir,
    'ppc': default_ppc_imgdir
}
default_defines = {
    'x86': x86_defines,
    'x64': x64_defines,
    'arm': arm_defines,
    'ppc': ppc_defines
}

# Architecture directory, in which generic architectural implementations can
# be found. Typically a 'common' architecture directory, where particular
# architecture versions are referenced as sub-architectures (see below).
default_arch_dir = {
    'x86': 'x86_common',
    'x64': 'x86_common',
    'arm': 'arm_common',
    'ppc': 'ppc_common',
    'hosted': 'hosted',
}

# Sub-architecture (e.g. x86_64 for x86) directory; provides more specific
# architecture implementations for particular intra-architectural differences.
# e.g. ARM -> ARMv7, X86 -> X86_64.
default_subarch_dir = {
    'x86': 'x86',
    'x64': 'x64',
    'arm': 'armv7',
}

# Machine directories.
default_machine_dir = {
    'pc': 'mach_pc',
    'mac': 'mac',
    'hosted': 'hosted',
    'beagle': 'arm_beagle',
}

# Extra configuration for each target.
# CUSTOM_MEMCPY: custom userspace memcpy instead of newlib's default
# NATIVE_SUBSYSTEM: build kernel side of the native subsystem
# NATIVE_SUBSYSTEM_USER: also build user side of the native subsystem
default_extra_config = {
    'x64': ['CUSTOM_MEMCPY', 'NATIVE_SUBSYSTEM', 'NATIVE_SUBSYSTEM_USER'],
    'hosted': ['CUSTOM_MEMCPY', 'NATIVE_SUBSYSTEM', 'NATIVE_SUBSYSTEM_USER'],
    'arm': ['NATIVE_SUBSYSTEM'],
}

# Boot directory for the target.
target_boot_directory = {
    'arm': 'arm',
    'ppc': 'ppc',
    'mips': 'mips',
}

# Environmental overrides forced by targets.
# e.g. 'arm': {'build_lgpl': False}
environment_overrides = {
    'arm': {'build_lgpl': False, 'build_apps': False, 'build_images': False},
    'ppc': {'build_lgpl': False, 'build_apps': False, 'build_images': False},
}
