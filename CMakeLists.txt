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
project(PEDIGREE C CXX ASM-ATT)
cmake_minimum_required(VERSION 2.7)

set(DEFINES DUMMY_ITEM DUMMY_ITEM)

include(buildFlags.cmake)

list(REMOVE_ITEM DEFINES DUMMY_ITEM)

option(COLORGCC "Use the colorgcc script to colourise the compiler output." ON)
set(BOOTLOADER_BUILT)

#########################################################################################
# X86 and X64 target options.
if(X86_COMMON)

  if(X64)
    set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -m64 -mno-red-zone -mno-sse -mcmodel=kernel")
    set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -m64 -mno-red-zone -mno-sse -mcmodel=kernel")
    set(PEDIGREE_ASFLAGS "-felf64")
    set(LINKER_SCRIPT
  "${PEDIGREE_SOURCE_DIR}/src/system/kernel/core/processor/x64/kernel.ld -nostdlib -nostdinc")
  else(X64)
    set(PEDIGREE_CFLAGS "-march=i486 -fno-builtin -fno-stack-protector -nostdlib -m32")
    set(PEDIGREE_CXXFLAGS "-march=i486 -Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -m32")
    set(PEDIGREE_ASFLAGS "-felf")
    set(LINKER_SCRIPT "${PEDIGREE_SOURCE_DIR}/src/system/kernel/core/processor/x86/kernel.ld -nostdlib -nostdinc")
  endif(X64)
  
endif(X86_COMMON)

#########################################################################################
# MIPS target options.
if(MIPS_COMMON)

  set(BOOTLOADER_BUILT bootloader)
  if(MIPS64)
    set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -G0 -mips32 -mabi=64 -mxgot -static -fomit-frame-pointer")
    set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -G0 -mips32 -mabi=64 -mxgot -static -fomit-frame-pointer")
    set(PEDIGREE_ASFLAGS "-G0 -mips64 -mabi=64")
  else(MIPS64)
    set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -G0 -mips32 -mabi=32 -mxgot -static -fomit-frame-pointer -mtune=r4000")
    set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -G0 -mips32 -mabi=32 -mxgot -static -fomit-frame-pointer -mtune=r4000")
    # NOTE: errors occur if you add more than one thing to ASFLAGS!
    set(PEDIGREE_ASFLAGS "-mips32")
    set(LINKER_SCRIPT "${PEDIGREE_SOURCE_DIR}/src/system/kernel/link-mips.ld -mxgot")
  endif(MIPS64)
  
endif(MIPS_COMMON)

#########################################################################################
# ARM target options.
if(ARM_COMMON)

  set(BOOTLOADER_BUILT bootloader)
  set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -fomit-frame-pointer")
  set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -fomit-frame-pointer")
  set(LINKER_SCRIPT "${PEDIGREE_SOURCE_DIR}/src/system/kernel/link-arm.ld -mxgot")
  
endif(ARM_COMMON)

#########################################################################################
# PPC target options.
if(PPC_COMMON)

  set(BOOTLOADER_BUILT bootloader)
  set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -fomit-frame-pointer")
  set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -pedantic -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -fomit-frame-pointer")
  set(LINKER_SCRIPT "${PEDIGREE_SOURCE_DIR}/src/system/kernel/link-ppc.ld")
  
endif(PPC_COMMON)

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
if(ARM_COMMON)
  set(TMP_TARGETS ${TMP_TARGETS} blah)
endif(ARM_COMMON)
if(PPC_COMMON)
  set(TMP_TARGETS ${TMP_TARGETS} blah)
endif(PPC_COMMON)

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
if(ARM_COMMON)
  set(COMPILER arm-elf)
endif(ARM_COMMON)
if(PPC_COMMON)
  set(COMPILER ppc-elf)
endif(PPC_COMMON)

if(DEBUGGER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -g3 -gdwarf-2")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -g3 -gdwarf-2")
else(DEBUGGER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -g0 -O3")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -g0 -O3")
endif(DEBUGGER)
if(OMIT_FRAME_POINTER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -fomit-frame-pointer")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -fomit-frame-pointer")
endif(OMIT_FRAME_POINTER)

# Add compiler #defines.
foreach(definition ${DEFINES})
  add_definitions(-D${definition}=1)
endforeach(definition)

# Uncomment this if you want to see the exact commands CMake runs.
set(CMAKE_VERBOSE_MAKEFILE ON)

# Remove the default -rdynamic option from the linker flags.
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
# Remove the default -fPIC option from the linker flags.
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)

# Set the C and C++ compiler lines.
if(COLORGCC)
  set(CMAKE_CXX_COMPILER ${PEDIGREE_SOURCE_DIR}/scripts/${COMPILER}-g++)
  set(CMAKE_C_COMPILER ${PEDIGREE_SOURCE_DIR}/scripts/${COMPILER}-gcc)
else(COLORGCC)
  set(CMAKE_CXX_COMPILER ${PEDIGREE_SOURCE_DIR}/compilers/bin/${COMPILER}-c++)
  set(CMAKE_C_COMPILER ${PEDIGREE_SOURCE_DIR}/compilers/bin/${COMPILER}-gcc)
endif(COLORGCC)

set(CMAKE_CXX_FLAGS ${PEDIGREE_CXXFLAGS})
set(CMAKE_C_FLAGS ${PEDIGREE_CFLAGS})
set(CMAKE_ASM-ATT_FLAGS ${PEDIGREE_ASFLAGS})

# Set the assembler.
if(X86_COMMON)
  set(CMAKE_ASM-ATT_COMPILER ${PEDIGREE_SOURCE_DIR}/compilers/dir/bin/nasm)
else(X86_COMMON)
  set(CMAKE_ASM-ATT_COMPILER ${PEDIGREE_SOURCE_DIR}/compilers/dir/bin/${COMPILER}-as)
endif(X86_COMMON)

# Create an emacs TAGS file.
add_custom_target(tags
                  find src/ -name *.h -or -name *.cc | etags -o TAGS -
		  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}
                  COMMENT "Creating Emacs tags file")

# Create a target used for checking if a compiler is present.
add_custom_target(check_compiler ALL perl ./scripts/checkBuildSystem.pl ${COMPILER}
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR})

# Create a target for creating the Version.cc file.
add_custom_target(version.cc ALL perl ./scripts/createVersionCc.pl ${DEFINES}
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR})

# Recurse into the Kernel directory first.
add_subdirectory(src/system/kernel)

add_dependencies(kernel version.cc check_compiler)

# Recurse into the Modules directory.
add_subdirectory(src/system/modules)

add_dependencies(initrd kernel)

# Then the bootloader.
add_subdirectory(src/system/boot)

# Create a target used for creating the image files.
add_custom_target(create_images ALL perl ../scripts/createImages.pl ${COMPILER}
                  DEPENDS kernel ${BOOTLOADER_BUILT}
                  WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR})

add_dependencies(create_images initrd)

include(CTest)
