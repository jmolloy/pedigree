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
# Helper macro to add sources to the kernel.
macro(add_kernel_src directory)
  set(src)
  foreach(var ${ARGV})
    if(NOT ${var} STREQUAL ${directory})
      set(src ${src} "${directory}/${var}")
    endif(NOT ${var} STREQUAL ${directory})
  endforeach(var ${ARGV})
  set(KERNEL_SRC ${KERNEL_SRC} ${src} PARENT_SCOPE)
  set(KERNEL_SRC ${KERNEL_SRC} ${src}) # Possible hack.
endmacro(add_kernel_src)

#########################################################################################
# Sources list.
set(KERNEL_SRC Log.cc Version.cc Archive.cc Spinlock.cc)
if(MIPS_COMMON)
  set(KERNEL_SRC ${KERNEL_SRC} boot-mips.cc)
endif(MIPS_COMMON)
if(ARM_COMMON)
  set(KERNEL_SRC ${KERNEL_SRC} boot-arm.cc)
endif(ARM_COMMON)

add_subdirectory(core)
add_subdirectory(debugger)
add_subdirectory(linker)
add_subdirectory(machine)
add_subdirectory(utilities)

#########################################################################################
# Script begin.

include_directories(../include
                    .
                    debugger
                    debugger/libudis86
                    debugger/commands
                    linker
                    core/lib
                    machine/x86_common/x86emu/include machine/x86_common/x86emu/src/x86emu)

add_definitions(-D__UD_STANDALONE__)

# One file needs to be assembled to binary on x86 and x64.
if(X86 AND MULTIPROCESSOR)
  add_custom_target(trampoline
                    COMMAND ${CMAKE_ASM-ATT_COMPILER} -f bin -o ${PEDIGREE_BINARY_DIR}/src/system/kernel/CMakeFiles/kernel.dir/core/processor/x86/asm/trampoline.bin ${PEDIGREE_SOURCE_DIR}/src/system/kernel/core/processor/x86/asm/trampoline.s)
endif(X86 AND MULTIPROCESSOR)
if(X64 AND MULTIPROCESSOR)
  add_custom_target(trampoline
                    COMMAND ${CMAKE_ASM-ATT_COMPILER} -f bin -o ${PEDIGREE_BINARY_DIR}/src/system/kernel/CMakeFiles/kernel.dir/core/processor/x64/asm/trampoline.bin ${PEDIGREE_SOURCE_DIR}/src/system/kernel/core/processor/x64/asm/trampoline.s)
endif(X64 AND MULTIPROCESSOR)

add_executable(kernel ${KERNEL_SRC})
target_link_libraries(kernel gcc)
set_target_properties(kernel PROPERTIES LINK_FLAGS
                      "-T ${LINKER_SCRIPT}")

install(TARGETS kernel
        RUNTIME DESTINATION "kernel/")

if(X86_COMMON AND MULTIPROCESSOR)
  add_dependencies(kernel
                   trampoline)
  if(X86)
    set_source_files_properties(core/processor/x86/asm/multiprocessor.s
                                PROPERTIES OBJECT_DEPENDS ${PEDIGREE_BINARY_DIR}/src/system/kernel/CMakeFiles/kernel.dir/core/processor/x86/asm/trampoline.bin)
  else(X86)
    set_source_files_properties(core/processor/x64/asm/multiprocessor.s
                                PROPERTIES OBJECT_DEPENDS ${PEDIGREE_BINARY_DIR}/src/system/kernel/CMakeFiles/kernel.dir/core/processor/x64/asm/trampoline.bin)
  endif(X86)
endif(X86_COMMON AND MULTIPROCESSOR)
