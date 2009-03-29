#!/usr/bin/env python

#########################################
# Setup our environment varibles
#########################################
Import('*')

# C Compiler
env['CC'] = '$COMPILER_PATH-gcc'
# C++ Compiler
env['CXX'] = '$COMPILER_PATH-g++'
# Assembler
if env['ARCH_FAMILY'] = 'x86':
  env['AS'] = 'nasm'
else:
  env['AS'] = '$COMPILER_PATH-as'
# Linker
env['LINK'] = '$COMPILER_PATH-ld'

#########################################
# Flags
#########################################

# X86 Shared flags.

# C/C++ shared flags
env['SHARED_FLAGS'] = '-march=i486 -fno-builtin -fno-stack-protector -nostdlib -m32 -g0 -O3'
# C flags
env['CFLAGS'] = '$SHARED_FLAGS'
# C++ flags
env['CXXFLAGS'] = '$SHARED_FLAGS -Weffc++ -Wall -Wold-style-cast -Wno-long-long -fno-rtti -fno-exceptions'
# Assembler flags
env['ASFLAGS'] = '-f elf'
# Linker flags
env['LINKFLAGS'] = '-T src/system/kernel/core/processor/x86/kernel.ld -nostdlib -nostdinc'
