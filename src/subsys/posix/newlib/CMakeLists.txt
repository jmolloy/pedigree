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

if (COMPILE_NEWLIB)

set(LIBC)
set(LIBM)
if(X86)
  set(LIBC "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-i686-libc.a")
  set(LIBM "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-i686-libm.a")
  set(CONFIGURE_HOST "--host=i686-elf")
  set(CONFIGURE_TARGET "--target=i686-elf")
  set(STUB i686-elf)
endif(X86)
if(PPC_COMMON)
  set(LIBC "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-ppc-libc.a")
  set(LIBM "${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib-ppc-libm.a")
  set(CONFIGURE_HOST "--host=ppc-elf")
  set(CONFIGURE_TARGET "--target=ppc-elf")
  set(STUB ppc-elf)
endif(PPC_COMMON)

if (EXISTS ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib/newlib-1.15.0/configure)
else (EXISTS ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib/newlib-1.15.0/configure)

  file(DOWNLOAD "ftp://sources.redhat.com/pub/newlib/newlib-1.15.0.tar.gz" ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib/newlib-1.15.0.tar.gz STATUS MYSTATUS)

  list(GET MYSTATUS 0 WASERROR)
  list(GET MYSTATUS 1 ERRSTR)
  if (NOT WASERR)
    execute_process(COMMAND tar -xzf newlib-1.15.0.tar.gz
                    WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib)
  else (NOT WASERR)
    message(FATAL_ERROR "Newlib failed to download: ${ERRSTR}")
  endif (NOT WASERR)

  execute_process(COMMAND cp configure.host newlib-1.15.0/newlib/configure.host
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib)
  execute_process(COMMAND cp ../include/sys/dirent.h newlib-1.15.0/newlib/libc/include/sys/dirent.h
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib)
  execute_process(COMMAND cp ../include/sys/utmp.h newlib-1.15.0/newlib/libc/include/sys/utmp.h
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib)
  execute_process(COMMAND cp ../include/sys/termios.h newlib-1.15.0/newlib/libc/include/sys/termios.h
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib)

endif (EXISTS ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib/newlib-1.15.0/configure)

set(CCPREFIX "${PEDIGREE_SOURCE_DIR}/compilers/dir/bin")

execute_process(COMMAND mkdir -p ${PEDIGREE_BINARY_DIR}/src/subsys/posix/newlib/build)
execute_process(COMMAND bash ${PEDIGREE_SOURCE_DIR}/src/subsys/posix/newlib/newlib-1.15.0/newlib/configure ${CONFIGURE_HOST} ${CONFIGURE_TARGET} CC=${CCPREFIX}/${STUB}-gcc CPP=${CCPREFIX}/${STUB}-cpp AS=${CCPREFIX}/${STUB}-as
                WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR}/src/subsys/posix/newlib/build)

add_custom_target(newlib ALL
                  COMMAND make
                  COMMAND cp libc.a ${PEDIGREE_BINARY_DIR}/libc.a
                  COMMAND cp libm.a ${PEDIGREE_BINARY_DIR}/libm.a
                  WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR}/src/subsys/posix/newlib/build
                  COMMENT "Compiling newlib - this may take several minutes/hours depending on your machine spec.")

endif (COMPILE_NEWLIB)
