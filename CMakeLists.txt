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

#########################################################################################
# The project name and CMake version.
project(PEDIGREE)
cmake_minimum_required(VERSION 2.4)

#########################################################################################
# Macros used to compose the options list.

macro(pedigree_option option_name description default)
  option(${option_name} ${description} ${default})
  if(${option_name})
    set(DEFINES ${DEFINES} ${option_name})
  endif(${option_name})
endmacro(pedigree_option)

macro(pedigree_set option_name)
  set(DEFINES ${DEFINES} ${option_name})
  set(${option_name} ON)
endmacro(pedigree_set)

#########################################################################################
# X86 and X64 target options.
option(X86_COMMON "Build for the IA32/AMD64 architecture." ON)
if(X86_COMMON)

  pedigree_set(X86_COMMON)
  pedigree_set(LITTLE_ENDIAN)
  option(X64 "64-bit mode? (OFF for 32-bit)" OFF)
  if(X64)
    pedigree_set(X64)
    pedigree_set(BITS_64)
  else(X64)
    pedigree_set(X86)
    pedigree_set(BITS_32)
  endif(X64)
  
  option(MULTIPROCESSOR "Enable multiprocessor support." OFF)
  if(MULTIPROCESSOR)
    pedigree_set(MULTIPROCESSOR)
    pedigree_option(SMP "Enable support for symmetric multiprocessing." OFF)
    pedigree_option(APIC "Enable support for APIC interrupt controllers." OFF)
  endif(MULTIPROCESSOR)

  pedigree_option(ACPI "Enable support for the advanced control and power interface." OFF)
  
endif(X86_COMMON)

#########################################################################################
# MIPS target options.
option(MIPS_COMMON  "Build for the MIPSel (R4000+) architecture." OFF)
if(MIPS_COMMON)

  pedigree_set(MIPS_COMMON)
  pedigree_set(LITTLE_ENDIAN)
  option(MIPS64 "64-bit mode? (OFF for 32-bit)" OFF)
  if(MIPS64)
    pedigree_set(BITS_64)
    pedigree_set(MIPS64)
  else(MIPS64)
    pedigree_set(BITS_32)
    pedigree_set(MIPS32)
  endif(MIPS64)

endif(MIPS_COMMON)

#########################################################################################
# Script start.

# Check that multiple targets aren't defined.
set(TMP_TARGETS blah)
if(X86_COMMON)
  set(TMP_TARGETS ${TMP_TARGETS} blah)
endif(X86_COMMON)
if(MIPS_COMMON)
  set(TMP_TARGETS ${TMP_TARGETS} blah)
endif(MIPS_COMMON)
list(LENGTH TMP_TARGETS TMP_NUM_TARGETS)
if(TMP_NUM_TARGETS GREATER 2)
  message(FATAL_ERROR "Multiple targets selected!")
endif(TMP_NUM_TARGETS GREATER 2)
if(TMP_NUM_TARGETS LESS 2)
  message(FATAL_ERROR "No targets selected!")
endif(TMP_NUM_TARGETS LESS 2)

# Attempt to set a compiler name.
if(X86)
  set(COMPILER i686-elf)
endif(X86)
if(X64)
  set(COMPILER amd64-elf)
endif(X64)
if(MIPS_COMMON)
  set(COMPILER mips64el-elf)
endif(MIPS_COMMON)

# Create a target used for checking if a compiler is present.
add_custom_target(check_compiler ALL perl ./checkBuildSystem.pl ${COMPILER}
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/build)

# Recurse into the Kernel directory first.
add_subdirectory(src/system/kernel)

message("blah ${DEFINES}")
