# Default flags for Pedigree's build, per architecture

#------------------------- All Architectures -------------------------#

# Generic entry-level flags (that everyone should have)
generic_cflags = '-std=gnu99 -fno-builtin -nostdinc -nostdlib -ffreestanding -g0 -O3 '
generic_cxxflags = generic_cflags.replace('-std=gnu99', '-std=gnu++98') + ' -fno-rtti -fno-exceptions '

# Warning flags (that force us to write betterish code)
warning_flags = '-Wall -Wextra -Wpointer-arith -Wcast-align -Wwrite-strings -Wno-long-long -Wno-variadic-macros '

# Language-specific warnings
warning_flags_c = '-Wnested-externs '
warning_flags_cxx = ''

# Turn off some warnings (because they kill the build)
warning_flags_off = '-Wno-unused -Wno-unused-variable -Wno-conversion -Wno-format -Wno-empty-body'

# Generic assembler flags
generic_asflags = '-f elf'

# Generic link flags (that everyone should have)
generic_linkflags = '-nostdlib -nostdinc -nostartfiles'

#------------------------- x86 (+x64) -------------------------#

# x86 defines - udis86 uses __UD_STANDALONE__
general_x86_defines = ['X86_COMMON', 'LITTLE_ENDIAN', '__UD_STANDALONE__']

#---------- x86, 32-bit ----------#

# 32-bit defines
x86_32_defines = ['X86', 'BITS_32', 'KERNEL_NEEDS_ADDRESS_SPACE_SWITCH']

# 32-bit CFLAGS and CXXFLAGS
default_x86_cflags = ' -march=i486 '
default_x86_cxxflags = ' -march=i486 '

# 32-bit assembler flags
default_x86_asflags = '32'

# 32-bit linker flags
default_x86_linkflags = ' -T src/system/kernel/core/processor/x86/kernel.ld '

# x86 32-bit images directory
default_x86_imgdir = '#/images/x86/'

# x86 final variables
x86_cflags = default_x86_cflags + generic_cflags + warning_flags + warning_flags_c + warning_flags_off
x86_cxxflags = default_x86_cxxflags + generic_cxxflags + warning_flags + warning_flags_cxx + warning_flags_off
x86_asflags = generic_asflags + default_x86_asflags
x86_linkflags = generic_linkflags + default_x86_linkflags
x86_defines = general_x86_defines + x86_32_defines

#---------- x86_64 ----------#

# x64 defines
x64_defines = ['X64', 'BITS_64']

# x64 CFLAGS and CXXFLAGS
default_x64_cflags = ' -m64 -mno-red-zone -mcmodel=kernel '
default_x64_cxxflags = ' -m64 -mno-red-zone -mcmodel=kernel '

# x64 assembler flags
default_x64_asflags = '64'

# x64 linker flags
default_x64_linkflags = ' -T src/system/kernel/core/processor/x64/kernel.ld '

# x64 images directory
default_x64_imgdir = '#/images/x64/'

# x64 final variables
x64_cflags = default_x64_cflags + generic_cflags + warning_flags + warning_flags_c + warning_flags_off
x64_cxxflags = default_x64_cxxflags + generic_cxxflags + warning_flags + warning_flags_cxx + warning_flags_off
x64_asflags = generic_asflags + default_x64_asflags
x64_linkflags = generic_linkflags + default_x64_linkflags
x64_defines = general_x86_defines + x64_defines

#------------------------- Import Dictionaries -------------------------#
default_cflags      = {'x86' : x86_cflags, 'x64' : x64_cflags }
default_cxxflags    = {'x86' : x86_cxxflags, 'x64' : x64_cxxflags }
default_asflags     = {'x86' : x86_asflags, 'x64' : x64_asflags }
default_linkflags   = {'x86' : x86_linkflags, 'x64' : x64_linkflags }
default_imgdir      = {'x86' : default_x86_imgdir, 'x64' : default_x64_imgdir }
default_defines     = {'x86' : x86_defines, 'x64' : x64_defines }
