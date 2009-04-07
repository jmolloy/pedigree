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

# The posix subsystem has two targets - the user mode library which should be linked
# with the libc, and the kernel mode code which should be made into a module.

set(LIBC)
set(LIBM)
if(X86)
  set(LIBC "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-i686-libc.a")
  set(LIBM "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-i686-libm.a")
  set(STUB i686-elf)
  set(SIGRET "i686")
endif(X86)
if(PPC_COMMON)
  set(LIBC "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-ppc-libc.a")
  set(LIBM "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-ppc-libm.a")
  set(STUB ppc-elf)
  set(SIGRET "ppc")
endif(PPC_COMMON)

include_directories(../../system/include ../../system/kernel/linker ../../system/modules/ ./ ./include)

# Do the kernel module first.
add_executable(posix.o posix.cc PosixSyscallManager.cc file-syscalls.cc system-syscalls.cc console-syscalls.cc net-syscalls.cc pipe-syscalls.cc sigret-${SIGRET}.s DevFs.cc)
target_link_libraries(posix.o gcc)
set_target_properties(posix.o PROPERTIES
                              LINK_FLAGS "-r -T ${PEDIGREE_SOURCE_DIR}/src/drivers/link.ld")

add_executable(crt0.o crt0.c)
set_target_properties(crt0.o PROPERTIES
                             LINK_FLAGS "-r")

install(TARGETS crt0.o
        RUNTIME DESTINATION "kernel/")

# include_directories(./include ../../system/include ../../system/kernel/linker ../../system/modules/ ./)

# Now the glue code - compile to a .o...
add_executable(pedigree-glue.o glue.c glue-sysconf.c)
target_link_libraries(pedigree-glue.o gcc)
set_target_properties(pedigree-glue.o PROPERTIES
                                      LINK_FLAGS "-r")

get_filename_component(COMPILER_DIR ${C_COMPILER} PATH)

add_custom_target(posix-usermode ALL perl ${PEDIGREE_SOURCE_DIR}/scripts/posixBlendLibc.pl ${LIBC} ${LIBM} "${PEDIGREE_BINARY_DIR}/src/subsys/posix/pedigree-glue.o" "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/include" "${CMAKE_C_COMPILER}" ${PEDIGREE_BINARY_DIR} "${C_COMPILER}/../lib/gcc/${STUB}/4.3.0/"
                  DEPENDS pedigree-glue.o
		  WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR})
