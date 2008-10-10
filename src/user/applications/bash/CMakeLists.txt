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

if (X86)
  set (BASH_PREFIX "i686-elf")
endif (X86)
if (PPC_COMMON)
  set (BASH_PREFIX "ppc-elf")
endif (PPC_COMMON)

if (COMPILE_BASH)

  if (EXISTS ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2/configure)
  else (EXISTS ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2/configure)

    file(DOWNLOAD "ftp://ftp.gnu.org/gnu/bash/bash-3.2.tar.gz" ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2.tar.gz STATUS MYSTATUS)

    list(GET MYSTATUS 0 WASERROR)
    list(GET MYSTATUS 1 ERRSTR)
    if (NOT WASERR)
      execute_process(COMMAND tar -xzf bash-3.2.tar.gz
        WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash)
      execute_process(COMMAND patch -i ../no_main_env_arg.patch
	WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2)
    else (NOT WASERR)
      message(FATAL_ERROR "Bash failed to download: ${WASERR}, ${ERRSTR}")
    endif (NOT WASERR)

  endif (EXISTS ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2/configure)

  set(MYPATH "$ENV{PATH}:${PEDIGREE_SOURCE_DIR}/compilers/dir/bin")

  set(MY_CFLAGS "-I ${PEDIGREE_BINARY_DIR}/include-posix/ -fno-builtin -DNO_MAIN_ENV_ARG")
  set(MY_LDFLAGS "-L ${PEDIGREE_BINARY_DIR}")

  execute_process(COMMAND mkdir -p ${PEDIGREE_BINARY_DIR}/src/user/applications/bash/build)
  execute_process(COMMAND bash ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash/bash-3.2/configure --host=${BASH_PREFIX} --target=${BASH_PREFIX} "CFLAGS=${MY_CFLAGS}" "CPPFLAGS=${MY_CFLAGS}" "LDFLAGS=${MY_LDFLAGS}" PATH=${MYPATH}
    WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR}/src/user/applications/bash/build)

  add_custom_target(bash ALL
    COMMAND make PATH=${MYPATH}
    COMMAND cp bash ../bash
    WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR}/src/user/applications/bash/build
    COMMENT "Compiling GNU bash - this may take several minutes/hours depending on your machine spec.")

else (COMPILE_BASH)

  # Just copy the precompiled bash.
  add_custom_target(bash ALL mkdir -p ${PEDIGREE_BINARY_DIR}/src/user/applications/bash
    COMMAND cp ${BASH_PREFIX}-bash ${PEDIGREE_BINARY_DIR}/src/user/applications/bash/bash
    WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/user/applications/bash)

endif (COMPILE_BASH)
